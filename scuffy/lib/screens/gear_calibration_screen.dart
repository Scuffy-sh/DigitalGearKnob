import 'package:flutter/material.dart';

import '../models/calibration_state.dart';
import '../services/ble_service.dart';

import 'dart:async';

class GearCalibrationResult {
  final double w;
  final double x;
  final double y;
  final double z;

  const GearCalibrationResult({
    required this.w,
    required this.x,
    required this.y,
    required this.z,
  });
}

class GearCalibrationScreen extends StatefulWidget {
  final String gear;
  final BleService bluetooth;
  final GearCalibrationResult? savedCalibration;
  final Color? accentColor;

  const GearCalibrationScreen({
    super.key,
    required this.gear,
    required this.bluetooth,
    this.savedCalibration,
    this.accentColor,
  });

  @override
  State<GearCalibrationScreen> createState() => _GearCalibrationScreenState();
}

class _GearCalibrationScreenState extends State<GearCalibrationScreen> {
  CalibrationState state = CalibrationState.waiting;

  double w = 0.0000;
  double x = 0.0000;
  double y = 0.0000;
  double z = 0.0000;

  String stability = "Esperando";
  GearCalibrationResult? savedCalibration;

  StreamSubscription? quaternionSubscription;
  StreamSubscription? calibrationSubscription;

  @override
  void initState() {
    super.initState();

    savedCalibration = widget.savedCalibration;
    if (savedCalibration != null) {
      w = savedCalibration!.w;
      x = savedCalibration!.x;
      y = savedCalibration!.y;
      z = savedCalibration!.z;
      stability = "Valores guardados";
    }

    quaternionSubscription = widget.bluetooth.quaternionStream.listen((q) {
      if (!mounted) return;

      if (state == CalibrationState.receiving) {
        setState(() {
          w = q.w;
          x = q.x;
          y = q.y;
          z = q.z;
        });
      }
    });

    calibrationSubscription = widget.bluetooth.calibrationFinished.listen((_) async {
      if (!mounted) return;

      setState(() {
        state = CalibrationState.completed;
        stability = "Posicion guardada";
        savedCalibration = GearCalibrationResult(w: w, x: x, y: y, z: z);
      });

      await widget.bluetooth.stopStream();
    });
  }

  @override
  void dispose() {
    quaternionSubscription?.cancel();
    calibrationSubscription?.cancel();

    widget.bluetooth.stopStream();

    super.dispose();
  }

  Future<void> startCalibration() async {
    setState(() {
      state = CalibrationState.receiving;
      stability = "Excelente";
    });

    await widget.bluetooth.startStream();
  }

  Future<void> saveCalibration() async {
    setState(() {
      state = CalibrationState.saving;
    });

    await widget.bluetooth.sendCalibration(widget.gear);
  }

  void returnToSetup() {
    Navigator.pop(context, savedCalibration);
  }

  Color get statusColor {
    switch (state) {
      case CalibrationState.waiting:
        return Colors.grey;

      case CalibrationState.receiving:
        return Colors.green;

      case CalibrationState.saving:
        return Colors.orange;

      case CalibrationState.completed:
        return Colors.green;
    }
  }

  Widget buildCard({required String title, required Widget child}) {
    return Card(
      color: const Color(0xFF1B1B1B),
      elevation: 3,
      shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(18)),
      child: Padding(
        padding: const EdgeInsets.all(18),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              title,
              style: const TextStyle(fontSize: 17, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 15),
            child,
          ],
        ),
      ),
    );
  }

  Widget buildQuaternionValue(String label, double value) {
    return Expanded(
      child: Container(
        padding: const EdgeInsets.symmetric(vertical: 18),
        decoration: BoxDecoration(
          color: const Color(0xFF262626),
          borderRadius: BorderRadius.circular(14),
        ),
        child: Column(
          children: [
            Text(
              label,
              style: const TextStyle(
                color: Colors.white70,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(height: 10),
            Text(
              value.toStringAsFixed(4),
              style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
            ),
          ],
        ),
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0B0B0B),

      appBar: AppBar(
        title: Text("Calibración marcha ${widget.gear}"),
        centerTitle: true,
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: returnToSetup,
        ),
      ),

      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(20),
          child: Column(
            children: [
              const Text(
                "Coloca la palanca en la marcha seleccionada antes de comenzar la calibración.",
                textAlign: TextAlign.center,
              ),

              const SizedBox(height: 25),

              buildCard(
                title: "Estado",
                child: Row(
                  children: [
                    Icon(Icons.circle, color: statusColor, size: 14),

                    const SizedBox(width: 10),

                    Text(
                      state.title,
                      style: const TextStyle(
                        fontSize: 16,
                        fontWeight: FontWeight.w600,
                      ),
                    ),
                  ],
                ),
              ),

              const SizedBox(height: 15),

              buildCard(
                title: "Orientación",
                child: state == CalibrationState.waiting && savedCalibration == null
                    ? const Text(
                        "Los valores aparecerán al iniciar la calibración.",
                        style: TextStyle(color: Colors.white70),
                      )
                    : Column(
                        children: [
                          Row(
                            children: [
                              buildQuaternionValue("W", w),
                              const SizedBox(width: 10),
                              buildQuaternionValue("X", x),
                            ],
                          ),
                          const SizedBox(height: 10),
                          Row(
                            children: [
                              buildQuaternionValue("Y", y),
                              const SizedBox(width: 10),
                              buildQuaternionValue("Z", z),
                            ],
                          ),
                        ],
                      ),
              ),

              const SizedBox(height: 15),

              buildCard(
                title: "Estabilidad",
                child: Row(
                  children: [
                    Icon(
                      Icons.check_circle,
                      color: state == CalibrationState.receiving
                          ? Colors.green
                          : Colors.grey,
                    ),

                    const SizedBox(width: 10),

                    Text(stability, style: const TextStyle(fontSize: 16)),
                  ],
                ),
              ),

              const Spacer(),

              if (state == CalibrationState.waiting)
                SizedBox(
                  width: double.infinity,
                  height: 55,
                  child: ElevatedButton(
                    onPressed: startCalibration,
                    style: ButtonStyle(
                      backgroundColor: WidgetStateProperty.all(widget.accentColor),
                    ),
                    child: const Text("Iniciar calibración"),
                  ),
                ),

              if (state == CalibrationState.receiving)
                SizedBox(
                  width: double.infinity,
                  height: 55,
                  child: ElevatedButton(
                    onPressed: saveCalibration,
                    style: ButtonStyle(
                      backgroundColor: WidgetStateProperty.all(widget.accentColor),
                    ),
                    child: const Text("Guardar posición"),
                  ),
                ),

              if (state == CalibrationState.saving)
                Column(
                  children: [
                    CircularProgressIndicator(
                      color: widget.accentColor,
                    ),
                    const SizedBox(height: 20),
                    const Text("Guardando posición..."),
                  ],
                ),

              if (state == CalibrationState.completed)
                SizedBox(
                  width: double.infinity,
                  height: 55,
                  child: ElevatedButton(
                    onPressed: startCalibration,
                    style: ButtonStyle(
                      backgroundColor: WidgetStateProperty.all(widget.accentColor),
                    ),
                    child: const Text("Calibrar de nuevo"),
                  ),
                ),
            ],
          ),
        ),
      ),
    );
  }
}
