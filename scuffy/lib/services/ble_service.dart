import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

class BleService {
  //=====================================================
  // SINGLETON
  //=====================================================

  static final BleService _instance = BleService._internal();
  factory BleService() => _instance;
  BleService._internal();

  //=====================================================
  // UUIDs
  //=====================================================

  static const String serviceUuid = "8b7d0001-5d6b-4d5d-a7b5-6d4b4c000001";

  static const String commandUuid = "8b7d0002-5d6b-4d5d-a7b5-6d4b4c000001";

  BluetoothDevice? device;
  BluetoothCharacteristic? _commandCharacteristic;
  bool _isConnected = false;

  //=====================================================
  // STREAMS (protocolo v2 — design "Protocolo BLE v2")
  //=====================================================

  final StreamController<bool> _connectionStateController =
      StreamController<bool>.broadcast();

  final StreamController<String> _themeColorController =
      StreamController.broadcast();

  final StreamController<String> _debugController =
      StreamController.broadcast();

  // Líneas completas de sniff ("sniff:<E|S><8hexid>:<datahex>" y la
  // alerta del watchdog "can:no_frames"). Se parsean con parseSniffFrame
  // / SniffSession en la capa de vista.
  final StreamController<String> _sniffController =
      StreamController.broadcast();

  // Estado del bus CAN: true = can:ok, false = can:offline, null = desconocido.
  final StreamController<bool?> _canStatusController =
      StreamController.broadcast();

  final StreamController<int> _protocolVersionController =
      StreamController.broadcast();

  Stream<bool> get connectionState => _connectionStateController.stream;
  Stream<String> get themeColorStream => _themeColorController.stream;
  Stream<String> get debugStream => _debugController.stream;
  Stream<String> get sniffStream => _sniffController.stream;
  Stream<bool?> get canStatusStream => _canStatusController.stream;
  Stream<int> get protocolVersionStream => _protocolVersionController.stream;

  StreamSubscription<List<int>>? _notifySubscription;
  StreamSubscription<BluetoothConnectionState>? _connectionStateSubscription;

  //=====================================================
  // PERMISOS
  //=====================================================

  Future<bool> requestPermissions() async {
    await [
      Permission.bluetoothScan,
      Permission.bluetoothConnect,
      Permission.location,
    ].request();

    return await Permission.bluetoothScan.isGranted &&
        await Permission.bluetoothConnect.isGranted;
  }

  //=====================================================
  // ESCANEAR Y CONECTAR
  //=====================================================

  Future<BluetoothDevice?> scanAndConnect() async {
    await FlutterBluePlus.stopScan();

    FlutterBluePlus.setLogLevel(LogLevel.verbose);

    BluetoothDevice? foundDevice;

    final subscription = FlutterBluePlus.scanResults.listen((results) {
      debugPrint("Resultados recibidos: ${results.length}");

      for (final result in results) {
        final name = result.device.platformName.isNotEmpty
            ? result.device.platformName
            : result.advertisementData.advName;

        debugPrint("Encontrado -> $name (${result.device.remoteId})");

        if (name == "SCUFFY" && foundDevice == null) {
          foundDevice = result.device;
        }
      }
    });

    await FlutterBluePlus.startScan(timeout: const Duration(seconds: 5));

    await FlutterBluePlus.isScanning.where((v) => !v).first;

    await subscription.cancel();

    if (foundDevice == null) return null;

    try {
      await foundDevice!.connect(timeout: const Duration(seconds: 10));
    } catch (_) {
      // Already connected or failed
    }

    device = foundDevice;
    _isConnected = true;

    await _discoverCharacteristics();

    _listenConnectionState();

    return foundDevice;
  }

  //=====================================================
  // DESCUBRIR SERVICIOS
  //=====================================================

  Future<void> _discoverCharacteristics() async {
    if (device == null) return;

    final services = await device!.discoverServices();

    for (final service in services) {
      if (service.uuid.toString().toLowerCase() == serviceUuid.toLowerCase()) {
        for (final characteristic in service.characteristics) {
          if (characteristic.uuid.toString().toLowerCase() ==
              commandUuid.toLowerCase()) {
            _commandCharacteristic = characteristic;

            debugPrint("Característica BLE encontrada");

            await characteristic.setNotifyValue(true);

            _notifySubscription = characteristic.lastValueStream.listen((data) {
              _processNotification(data);
            });

            return;
          }
        }
      }
    }

    debugPrint("No se encontró la característica BLE");
  }

  //=====================================================
  // ESCUCHAR ESTADO DE CONEXIÓN
  //=====================================================

  void _listenConnectionState() {
    _connectionStateSubscription?.cancel();

    _connectionStateSubscription =
        device!.connectionState.listen((state) {
      final isConnected = state == BluetoothConnectionState.connected;
      _isConnected = isConnected;
      _connectionStateController.add(isConnected);

      if (!isConnected) {
        _commandCharacteristic = null;
        _notifySubscription?.cancel();
        _notifySubscription = null;
      }
    });
  }

  //=====================================================
  // PROCESAR NOTIFICACIONES BLE (protocolo v2)
  //=====================================================
  // v2: los flujos BNO (quat:, calibration_ok, cal_status:, cal_data:,
  // bno:ok/error) fueron removidos del firmware. Quedan: theme:, debug:,
  // sniff:*, can:ok|offline, can:no_frames y proto:N.

  void _processNotification(List<int> data) {
    if (data.isEmpty) return;

    // Los mensajes BLE son texto; ignoramos cualquier terminador nulo que
    // pudiera incluir un periférico para comparar los comandos de forma fiable.
    final message = String.fromCharCodes(data).split('\u0000').first;

    debugPrint("BLE Notify -> $message");

    // Theme color from ESP
    if (message.startsWith("theme:")) {
      final hex = message.substring(6);
      _themeColorController.add(hex);
      return;
    }

    // Debug data from ESP (payload bruto; se parsea con parseCanDebug)
    if (message.startsWith("debug:")) {
      _debugController.add(message.substring(6));
      return;
    }

    // Sniff frames y alerta de bus silencioso
    if (message.startsWith("sniff:") || message == "can:no_frames") {
      _sniffController.add(message);
      return;
    }

    // CAN bus status
    if (message == "can:ok") {
      _canStatusController.add(true);
      return;
    }

    if (message == "can:offline") {
      _canStatusController.add(false);
      return;
    }

    // Firmware protocol version
    if (message.startsWith("proto:")) {
      final version = int.tryParse(message.substring(6));
      if (version != null) {
        _protocolVersionController.add(version);
      }
      return;
    }

    // Cualquier otro mensaje (error:removed:* etc.) se ignora.
  }

  //=====================================================
  // ENVIAR COLOR
  //=====================================================

  Future<void> sendColor(Color color) async {
    if (_commandCharacteristic == null) {
      debugPrint("No hay característica BLE");
      return;
    }

    final hex =
        "#${color.toARGB32().toRadixString(16).substring(2).toUpperCase()}";

    final command = "set_color:$hex";

    debugPrint("Enviando -> $command");

    await _commandCharacteristic!.write(
      command.codeUnits,
      withoutResponse: false,
    );
  }

  //=====================================================
  // SNIFF (protocolo v2)
  //=====================================================

  Future<void> sendSniff(bool enabled) async {
    if (_commandCharacteristic == null) return;

    final command = enabled ? "sniff:on" : "sniff:off";

    debugPrint("Enviando -> $command");

    await _commandCharacteristic!.write(
      command.codeUnits,
      withoutResponse: false,
    );
  }

  //=====================================================
  // DEBUG MODE
  //=====================================================

  Future<void> startDebug() async {
    if (_commandCharacteristic == null) return;
    await _commandCharacteristic!.write(
      "debug:on".codeUnits,
      withoutResponse: false,
    );
  }

  Future<void> stopDebug() async {
    if (_commandCharacteristic == null) return;
    await _commandCharacteristic!.write(
      "debug:off".codeUnits,
      withoutResponse: false,
    );
  }

  //=====================================================
  // SOLICITAR ESTADO
  //=====================================================

  Future<void> requestState() async {
    if (_commandCharacteristic == null) return;

    await _commandCharacteristic!.write(
      "get_state".codeUnits,
      withoutResponse: false,
    );
  }

  //=====================================================
  // DESCONECTAR
  //=====================================================

  Future<void> disconnect() async {
    await _notifySubscription?.cancel();
    _notifySubscription = null;

    await _connectionStateSubscription?.cancel();
    _connectionStateSubscription = null;

    _canStatusController.add(null);
    _debugController.add('');

    if (device != null) {
      await device!.disconnect();
    }

    device = null;
    _commandCharacteristic = null;
    _isConnected = false;
    _connectionStateController.add(false);
  }

  //=====================================================
  // ESTADO
  //=====================================================

  bool get isConnected => _isConnected;
}
