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
import 'sniff_screen.dart';

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
  bool debugMode = false;
  String debugData = '';
  bool? canOnline; // null = desconocido; true = can:ok; false = can:offline
  int? protocolVersion;

  late TabController _tabController;

  // Marchas soportadas por el knob (referencia estática, sin calibración).
  static const List<Gear> gears = [
    Gear.reverse,
    Gear.first,
    Gear.second,
    Gear.third,
    Gear.fourth,
    Gear.fifth,
    Gear.neutral,
  ];

  // Theme streams
  StreamSubscription<bool>? _connectionStateSubscription;
  StreamSubscription<String>? _themeSubscription;

  // v2 status streams
  StreamSubscription<String>? _debugSubscription;
  StreamSubscription<bool?>? _canStatusSubscription;
  StreamSubscription<int>? _protocolSubscription;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);

    // Carga inicial (async, no bloquea el frame)
    _loadCachedState();

    // Suscripciones BLE diferidas al siguiente frame para
    // que la transición del splash sea fluida.
    SchedulerBinding.instance.addPostFrameCallback((_) {
      if (!mounted) return;
      _connectionStateSubscription =
          bluetooth.connectionState.listen((isConnected) {
        if (!mounted) return;
        setState(() {
          connected = isConnected;
          if (!isConnected) {
            canOnline = null;
            protocolVersion = null;
          }
        });
      });

      _themeSubscription = bluetooth.themeColorStream.listen((hex) {
        if (!mounted) return;
        final colorValue = int.parse(hex.replaceFirst('#', '0xFF'));
        final color = Color(colorValue);
        final matchIndex = scuffyThemes.indexWhere(
          (t) => !t.custom && t.color.toARGB32() == color.toARGB32(),
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

      _canStatusSubscription = bluetooth.canStatusStream.listen((online) {
        if (!mounted) return;
        setState(() {
          canOnline = online;
        });
      });

      _protocolSubscription =
          bluetooth.protocolVersionStream.listen((version) {
        if (!mounted) return;
        setState(() {
          protocolVersion = version;
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
    _debugSubscription?.cancel();
    _canStatusSubscription?.cancel();
    _protocolSubscription?.cancel();
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
        (t) => !t.custom && t.color.toARGB32() == Color(colorValue).toARGB32(),
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

  // ── Sniff ──────────────────────────────────────────────

  void _openSniffScreen() {
    Navigator.push(
      context,
      MaterialPageRoute(
        builder: (_) => SniffScreen(
          bluetooth: bluetooth,
          accentColor: customColor,
        ),
      ),
    );
  }

  // ── Build helpers ──────────────────────────────────────

  Widget _buildConnectionArea() {
    // Chip de estado del bus CAN (v2: reemplaza al chip IMU/BNO).
    final (canColor, canLabel) = switch (canOnline) {
      true => (customColor, "CAN: Conectado"),
      false => (Colors.red, "CAN: Sin datos"),
      null => (Colors.grey, "CAN: Sin datos"),
    };

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
                  color: !connected ? Colors.grey : canColor,
                ),
                const SizedBox(width: 6),
                Text(
                  canLabel,
                  style: TextStyle(
                    fontSize: 13,
                    color: !connected ? Colors.grey : canColor,
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
            const SizedBox(height: 8),
            SizedBox(
              width: double.infinity,
              height: 44,
              child: OutlinedButton.icon(
                onPressed: connected ? _openSniffScreen : null,
                icon: const Icon(Icons.radar, size: 20),
                label: const Text("SNIFF CAN"),
                style: OutlinedButton.styleFrom(
                  foregroundColor: customColor,
                  side: BorderSide(
                    color: customColor.withValues(alpha: 0.5),
                    width: 1,
                  ),
                  shape: RoundedRectangleBorder(
                    borderRadius: BorderRadius.circular(12),
                  ),
                ),
              ),
            ),

            // Aviso de versión de firmware (v2): si el knob no reporta
            // proto:2, el cliente queda desactualizado y avisa.
            if (connected && protocolVersion != 2) ...[
              const SizedBox(height: 8),
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(10),
                decoration: BoxDecoration(
                  color: Colors.orange.withValues(alpha: 0.12),
                  borderRadius: BorderRadius.circular(10),
                  border: Border.all(
                    color: Colors.orange.withValues(alpha: 0.4),
                    width: 1,
                  ),
                ),
                child: const Row(
                  children: [
                    Icon(Icons.system_update_alt,
                        size: 16, color: Colors.orange),
                    SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        "Firmware del knob desactualizado (se espera v2)",
                        style: TextStyle(fontSize: 12, color: Colors.orange),
                      ),
                    ),
                  ],
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

  Widget _buildMarchasTab() {
    return ListView(
      padding: const EdgeInsets.all(20),
      children: [
        const Text(
          "Marchas del knob",
          style: TextStyle(fontSize: 20, fontWeight: FontWeight.bold),
        ),
        const SizedBox(height: 4),
        const Text(
          "Referencia de posiciones (la detección es automática por CAN).",
          style: TextStyle(color: Colors.white54, fontSize: 13),
        ),
        const SizedBox(height: 16),
        for (final gear in gears) ...[
          GearCard(
            gear: gear.name,
            accentColor: customColor,
          ),
          const SizedBox(height: 12),
        ],
      ],
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
                    _buildMarchasTab(),
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
            Tab(icon: Icon(Icons.settings), text: "Marchas"),
          ],
        ),
      ),
    );
  }
}
