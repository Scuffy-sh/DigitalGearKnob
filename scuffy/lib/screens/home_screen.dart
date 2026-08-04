import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter/scheduler.dart';
import 'package:flutter_colorpicker/flutter_colorpicker.dart';

import '../data/themes.dart';
import '../models/gear.dart';
import '../models/theme_model.dart';
import '../services/ble_service.dart';
import '../services/state_storage.dart';
import '../widgets/gear_card.dart';
import '../widgets/theme_card.dart';
import 'gear_calibration_screen.dart';

class HomeScreen extends StatefulWidget {
  const HomeScreen({super.key});

  @override
  State<HomeScreen> createState() => _HomeScreenState();
}

class _HomeScreenState extends State<HomeScreen>
    with SingleTickerProviderStateMixin {
  final BleService bluetooth = BleService();

  int selectedTheme = 0;
  Color customColor = const Color(0xFF0052FF);
  bool connected = false;
  bool scanning = false;
  bool bnoAvailable = false;
  bool debugMode = false;
  String debugData = '';

  late TabController _tabController;

  // Theme streams
  StreamSubscription<bool>? _connectionStateSubscription;
  StreamSubscription<String>? _themeSubscription;

  // Calibration streams (for effects tab)
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
  StreamSubscription<bool>? _bnoStatusSubscription;
  StreamSubscription<String>? _debugSubscription;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);

    // Carga inicial (async, no bloquea el frame)
    _loadCachedState();
    _loadCachedCalibrations();

    // Suscripciones BLE diferidas al siguiente frame para
    // que la transición del splash sea fluida.
    SchedulerBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      _connectionStateSubscription =
          bluetooth.connectionState.listen((isConnected) {
        if (!mounted) return;
        setState(() {
          connected = isConnected;
          if (!isConnected) bnoAvailable = false;
        });
      });

      _themeSubscription = bluetooth.themeColorStream.listen((hex) {
        if (!mounted) return;
        final colorValue = int.parse(hex.replaceFirst('#', '0xFF'));
        final color = Color(colorValue);
        final matchIndex = scuffyThemes.indexWhere(
          (t) => !t.custom && t.color.value == color.value,
        );
        setState(() {
          if (matchIndex >= 0) {
            selectedTheme = matchIndex;
            customColor = scuffyThemes[matchIndex].color;
          } else {
            customColor = color;
            selectedTheme = scuffyThemes.indexWhere((t) => t.custom);
          }
        });
        StateStorage.saveThemeColor(hex);
      });

      _calibrationStatusSubscription =
          bluetooth.calibrationStatusStream.listen((status) {
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
          bluetooth.calibrationDataStream.listen((data) {
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

      _bnoStatusSubscription =
          bluetooth.bnoStatusStream.listen((available) {
        if (!mounted) return;
        setState(() {
          bnoAvailable = available;
        });
      });

      _debugSubscription = bluetooth.debugStream.listen((data) {
        if (!mounted) return;
        setState(() {
          debugData = data;
        });
      });
    });

    if (bluetooth.isConnected) {
      bluetooth.requestState();
    }
  }

  @override
  void dispose() {
    _tabController.dispose();
    _connectionStateSubscription?.cancel();
    _themeSubscription?.cancel();
    _calibrationStatusSubscription?.cancel();
    _calibrationDataSubscription?.cancel();
    _bnoStatusSubscription?.cancel();
    _debugSubscription?.cancel();
    super.dispose();
  }

  // ── Bluetooth ──────────────────────────────────────────

  Future<void> scanBluetooth() async {
    if (scanning) return;

    setState(() {
      scanning = true;
    });

    final granted = await bluetooth.requestPermissions();

    if (!mounted) return;

    if (!granted) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text("Permisos Bluetooth denegados")),
      );

      setState(() {
        scanning = false;
      });

      return;
    }

    final device = await bluetooth.scanAndConnect();

    if (!mounted) return;

    if (device != null) {
      ScaffoldMessenger.of(context).showSnackBar(
        SnackBar(content: Text("Conectado a ${device.platformName}")),
      );
      await bluetooth.requestState();
    } else {
      ScaffoldMessenger.of(
        context,
      ).showSnackBar(const SnackBar(content: Text("SCUFFY no encontrado")));
    }

    setState(() {
      scanning = false;
    });
  }

  // ── Theme ──────────────────────────────────────────────

  Future<void> _sendTheme(int index) async {
    if (!connected) return;

    setState(() {
      selectedTheme = index;
      customColor = scuffyThemes[index].color;
    });

    await bluetooth.sendColor(customColor);
  }

  Future<void> _openColorPicker() async {
    if (!connected) return;

    Color pickerColor = customColor;

    await showDialog(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text("Personalizar color"),
          content: SingleChildScrollView(
            child: ColorPicker(
              pickerColor: pickerColor,
              onColorChanged: (color) {
                pickerColor = color;
              },
              enableAlpha: false,
              displayThumbColor: true,
              pickerAreaHeightPercent: 0.8,
            ),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text("Cancelar"),
            ),
            ElevatedButton(
              onPressed: () async {
                setState(() {
                  customColor = pickerColor;
                  selectedTheme = scuffyThemes.indexWhere(
                    (theme) => theme.custom,
                  );
                });

                Navigator.pop(context);

                await bluetooth.sendColor(customColor);
              },
              child: const Text("Aplicar"),
            ),
          ],
        );
      },
    );
  }

  Future<void> _loadCachedState() async {
    final cachedColor = await StateStorage.loadThemeColor();
    if (cachedColor != null && mounted) {
      final colorValue = int.parse(cachedColor.replaceFirst('#', '0xFF'));
      final matchIndex = scuffyThemes.indexWhere(
        (t) => !t.custom && t.color.value == Color(colorValue).value,
      );
      setState(() {
        if (matchIndex >= 0) {
          selectedTheme = matchIndex;
          customColor = scuffyThemes[matchIndex].color;
        } else {
          customColor = Color(colorValue);
          selectedTheme = scuffyThemes.indexWhere((t) => t.custom);
        }
      });
    }
  }

  // ── Calibration ────────────────────────────────────────

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

  // ── Build helpers ──────────────────────────────────────

  Widget _buildConnectionArea() {
    return Padding(
      padding: const EdgeInsets.symmetric(horizontal: 20),
      child: Container(
        width: double.infinity,
        padding: const EdgeInsets.symmetric(vertical: 16, horizontal: 20),
        decoration: BoxDecoration(
          gradient: LinearGradient(
            begin: Alignment.topLeft,
            end: Alignment.bottomRight,
            colors: [
              Colors.white.withValues(alpha: 0.08),
              Colors.white.withValues(alpha: 0.03),
            ],
          ),
          borderRadius: BorderRadius.circular(18),
          border: Border.all(
            color: Colors.white.withValues(alpha: 0.1),
            width: 1,
          ),
          boxShadow: [
            BoxShadow(
              color: Colors.black.withValues(alpha: 0.2),
              blurRadius: 6,
              offset: const Offset(0, 2),
            ),
          ],
        ),
        child: Column(
          children: [
            SizedBox(
              width: double.infinity,
              height: 48,
              child: ElevatedButton.icon(
                onPressed: scanning ? null : scanBluetooth,
                icon: Icon(
                  connected ? Icons.bluetooth_connected : Icons.bluetooth_searching,
                ),
                label: Text(
                  connected ? "CONECTADO" : "CONECTAR",
                  style: const TextStyle(fontWeight: FontWeight.w600),
                ),
                style: ButtonStyle(
                  backgroundColor: WidgetStateProperty.all(customColor),
                  shape: WidgetStateProperty.all(
                    RoundedRectangleBorder(
                      borderRadius: BorderRadius.circular(14),
                    ),
                  ),
                ),
              ),
            ),
            const SizedBox(height: 8),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.circle,
                  size: 8,
                  color: connected ? Colors.green : Colors.grey,
                ),
                const SizedBox(width: 6),
                Text(
                  connected ? "BT: Conectado" : "BT: Desconectado",
                  style: TextStyle(
                    fontSize: 13,
                    color: connected ? Colors.green : Colors.grey,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 4),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.circle,
                  size: 8,
                  color: !connected
                      ? Colors.grey
                      : bnoAvailable
                          ? customColor
                          : Colors.red,
                ),
                const SizedBox(width: 6),
                Text(
                  !connected
                      ? "IMU: Sin datos"
                      : bnoAvailable
                          ? "IMU: Conectado"
                          : "IMU: Desconectado",
                  style: TextStyle(
                    fontSize: 13,
                    color: !connected
                        ? Colors.grey
                        : bnoAvailable
                            ? customColor
                            : Colors.red,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 8),
            Row(
              mainAxisAlignment: MainAxisAlignment.center,
              children: [
                Icon(
                  Icons.bug_report,
                  size: 8,
                  color: debugMode ? customColor : Colors.grey,
                ),
                const SizedBox(width: 6),
                GestureDetector(
                  onTap: () async {
                    debugMode = !debugMode;
                    if (debugMode) {
                      await bluetooth.startDebug();
                    } else {
                      await bluetooth.stopDebug();
                      setState(() {
                        debugData = '';
                      });
                    }
                    setState(() {});
                  },
                  child: Text(
                    debugMode ? "DEBUG: ON" : "DEBUG: OFF",
                    style: TextStyle(
                      fontSize: 13,
                      color: debugMode ? customColor : Colors.grey,
                      fontWeight:
                          debugMode ? FontWeight.w600 : FontWeight.normal,
                    ),
                  ),
                ),
              ],
            ),
            if (debugMode && debugData.isNotEmpty) ...[
              const SizedBox(height: 8),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.black.withValues(alpha: 0.4),
                  borderRadius: BorderRadius.circular(10),
                  border: Border.all(
                    color: customColor.withValues(alpha: 0.3),
                    width: 1,
                  ),
                ),
                child: Text(
                  debugData,
                  style: TextStyle(
                    fontSize: 14,
                    color: customColor,
                    fontFamily: 'monospace',
                    fontWeight: FontWeight.w600,
                  ),
                  textAlign: TextAlign.center,
                ),
              ),
            ],
          ],
        ),
      ),
    );
  }

  Widget _buildColorTab() {
    return ListView(
      padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 16),
      children: [
        const Text(
          "Temas",
          style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
        ),
        const SizedBox(height: 16),
        for (int i = 0; i < scuffyThemes.length; i++) ...[
          ThemeCard(
            theme: scuffyThemes[i].custom
                ? ScuffyTheme(
                    name: scuffyThemes[i].name,
                    color: customColor,
                    custom: true,
                  )
                : scuffyThemes[i],
            selected: selectedTheme == i,
            onTap: () {
              if (scuffyThemes[i].custom) {
                _openColorPicker();
              } else {
                _sendTheme(i);
              }
            },
          ),
          const SizedBox(height: 12),
        ],
      ],
    );
  }

  Widget _buildEffectsTab() {
    return ListView.builder(
      padding: const EdgeInsets.all(20),
      itemCount: gears.length,
      itemBuilder: (context, index) {
        final gear = gears[index];

        return Padding(
          padding: const EdgeInsets.only(bottom: 15),
          child: GearCard(
            gear: gear.gear.name,
            calibrated: gear.calibrated,
            accentColor: customColor,
            onTap: () async {
              final result = await Navigator.push<GearCalibrationResult>(
                context,
                MaterialPageRoute(
                  builder: (_) => GearCalibrationScreen(
                    gear: gear.gear.name,
                    bluetooth: bluetooth,
                    accentColor: customColor,
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
    );
  }

  // ── Main build ─────────────────────────────────────────

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0B0B0B),
      appBar: AppBar(
        title: const Text("SCUFFY"),
        centerTitle: true,
      ),
      body: Stack(
        children: [
          // Gradiente base del color del tema
          AnimatedContainer(
            duration: const Duration(milliseconds: 800),
            curve: Curves.easeInOut,
            decoration: BoxDecoration(
              gradient: LinearGradient(
                begin: Alignment.topCenter,
                end: Alignment.bottomCenter,
                colors: [
                  customColor.withValues(alpha: 0.08),
                  const Color(0xFF0B0B0B),
                ],
              ),
            ),
          ),
          // Glow radial en la parte superior
          AnimatedContainer(
            duration: const Duration(milliseconds: 800),
            curve: Curves.easeInOut,
            decoration: BoxDecoration(
              gradient: RadialGradient(
                center: const Alignment(0, -0.7),
                radius: 0.6,
                colors: [
                  customColor.withValues(alpha: 0.12),
                  Colors.transparent,
                ],
              ),
            ),
          ),
          // Contenido
          Column(
            children: [
              _buildConnectionArea(),
              const SizedBox(height: 8),
              Expanded(
                child: TabBarView(
                  controller: _tabController,
                  children: [
                    _buildColorTab(),
                    _buildEffectsTab(),
                  ],
                ),
              ),
            ],
          ),
        ],
      ),
      bottomNavigationBar: Container(
        decoration: BoxDecoration(
          color: customColor.withValues(alpha: 0.08),
          border: Border(
            top: BorderSide(
              color: customColor.withValues(alpha: 0.2),
              width: 1,
            ),
          ),
        ),
        child: TabBar(
          controller: _tabController,
          indicatorColor: customColor,
          labelColor: Colors.white,
          unselectedLabelColor: Colors.white54,
          tabs: const [
            Tab(icon: Icon(Icons.palette), text: "Color"),
            Tab(icon: Icon(Icons.tune), text: "Calibración"),
          ],
        ),
      ),
    );
  }
}
