import 'dart:async';

import 'package:flutter/material.dart';

import '../models/gear.dart';
import '../services/ble_service.dart';
import '../services/state_storage.dart';
import '../widgets/gear_card.dart';
import 'gear_calibration_screen.dart';

class GearSetupScreen extends StatefulWidget {
  final BleService bluetooth;
  final Color accentColor;

  const GearSetupScreen({super.key, required this.bluetooth, required this.accentColor});

  @override
  State<GearSetupScreen> createState() => _GearSetupScreenState();
}

class _GearSetupScreenState extends State<GearSetupScreen> {
  final List<GearInfo> gears = [
    GearInfo(gear: Gear.reverse),
    GearInfo(gear: Gear.first),
    GearInfo(gear: Gear.second),
    GearInfo(gear: Gear.third),
    GearInfo(gear: Gear.fourth),
    GearInfo(gear: Gear.fifth),
    GearInfo(gear: Gear.neutral),
  ];

  StreamSubscription<Map<String, bool>>? _calibrationStatusSubscription;
  StreamSubscription<Map<String, List<double>>>? _calibrationDataSubscription;

  @override
  void initState() {
    super.initState();
    _loadCachedCalibrations();

    _calibrationStatusSubscription =
        widget.bluetooth.calibrationStatusStream.listen((status) {
      if (!mounted) return;
      setState(() {
        for (final gear in gears) {
          final key = gear.gear.name;
          gear.calibrated = status[key] ?? false;
        }
      });
      StateStorage.saveCalibrationStatus(status);
    });

    _calibrationDataSubscription =
        widget.bluetooth.calibrationDataStream.listen((data) {
      if (!mounted) return;
      setState(() {
        for (final gear in gears) {
          final key = gear.gear.name;
          if (data.containsKey(key)) {
            final values = data[key]!;
            gear.calibrated = true;
            gear.w = values[0];
            gear.x = values[1];
            gear.y = values[2];
            gear.z = values[3];
            StateStorage.saveCalibrationData(
                key, values[0], values[1], values[2], values[3]);
          } else if (!gear.calibrated) {
            gear.w = 0;
            gear.x = 0;
            gear.y = 0;
            gear.z = 0;
          }
        }
      });
    });

    // Pedir estado fresco al ESP32 — los datos anteriores se perdieron
    // porque la pantalla no existía cuando se envió el último get_state.
    if (widget.bluetooth.isConnected) {
      widget.bluetooth.requestState();
    }
  }

  @override
  void dispose() {
    _calibrationStatusSubscription?.cancel();
    _calibrationDataSubscription?.cancel();
    super.dispose();
  }

  Future<void> _loadCachedCalibrations() async {
    final status = await StateStorage.loadCalibrationStatus();
    final data = await StateStorage.loadCalibrationData();

    if (!mounted) return;

    setState(() {
      for (final gear in gears) {
        final key = gear.gear.name;
        if (status[key] == true) {
          gear.calibrated = true;
          if (data != null && data.containsKey(key)) {
            gear.w = data[key]![0];
            gear.x = data[key]![1];
            gear.y = data[key]![2];
            gear.z = data[key]![3];
          }
        }
      }
    });
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0B0B0B),
      appBar: AppBar(
        title: const Text('Configuración de marchas'),
        centerTitle: true,
      ),
      body: ListView.builder(
        padding: const EdgeInsets.all(20),
        itemCount: gears.length,
        itemBuilder: (context, index) {
          final gear = gears[index];

          return Padding(
            padding: const EdgeInsets.only(bottom: 15),
            child: GearCard(
              gear: gear.gear.name,
              calibrated: gear.calibrated,
              accentColor: widget.accentColor,
              onTap: () async {
                final result = await Navigator.push<GearCalibrationResult>(
                  context,
                  MaterialPageRoute(
                    builder: (_) => GearCalibrationScreen(
                      gear: gear.gear.name,
                      bluetooth: widget.bluetooth,
                      savedCalibration: gear.calibrated
                          ? GearCalibrationResult(
                              w: gear.w,
                              x: gear.x,
                              y: gear.y,
                              z: gear.z,
                            )
                          : null,
                    ),
                  ),
                );

                if (result != null) {
                  setState(() {
                    gear.calibrated = true;
                    gear.w = result.w;
                    gear.x = result.x;
                    gear.y = result.y;
                    gear.z = result.z;
                  });
                }
              },
            ),
          );
        },
      ),
    );
  }
}
