import 'dart:async';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import '../models/quaternion_data.dart';

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
  // STREAMS
  //=====================================================

  final StreamController<QuaternionData> _quaternionController =
      StreamController.broadcast();

  final StreamController<void> _calibrationController =
      StreamController.broadcast();

  final StreamController<bool> _connectionStateController =
      StreamController<bool>.broadcast();

  Stream<QuaternionData> get quaternionStream => _quaternionController.stream;

  Stream<void> get calibrationFinished => _calibrationController.stream;

  Stream<bool> get connectionState => _connectionStateController.stream;

  final StreamController<String> _themeColorController =
      StreamController.broadcast();
  final StreamController<Map<String, bool>> _calibrationStatusController =
      StreamController.broadcast();
  final StreamController<Map<String, List<double>>>
      _calibrationDataController = StreamController.broadcast();
  final StreamController<bool> _bnoStatusController =
      StreamController<bool>.broadcast();
  final StreamController<String> _debugController =
      StreamController<String>.broadcast();

  Stream<String> get themeColorStream => _themeColorController.stream;
  Stream<Map<String, bool>> get calibrationStatusStream =>
      _calibrationStatusController.stream;
  Stream<Map<String, List<double>>> get calibrationDataStream =>
      _calibrationDataController.stream;
  Stream<bool> get bnoStatusStream => _bnoStatusController.stream;
  Stream<String> get debugStream => _debugController.stream;

  final Map<String, List<double>> _pendingCalData = {};
  int _pendingCalExpected = 0;
  Timer? _calDataTimer;

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
  // PROCESAR NOTIFICACIONES BLE
  //=====================================================

  void _processNotification(List<int> data) {
    if (data.isEmpty) return;

    // Los mensajes BLE son texto; ignoramos cualquier terminador nulo que
    // pudiera incluir un periférico para comparar los comandos de forma fiable.
    final message = String.fromCharCodes(data).split('\u0000').first;

    debugPrint("BLE Notify -> $message");

    // Quaternion
    if (message.startsWith("quat:")) {
      final values = message.substring(5).split(",");

      if (values.length != 4) return;
      try {
        _quaternionController.add(
          QuaternionData(
            w: double.parse(values[0]),
            x: double.parse(values[1]),
            y: double.parse(values[2]),
            z: double.parse(values[3]),
          ),
        );
      } catch (_) {}
    }

    // Calibración terminada
    if (message == "calibration_ok") {
      _calibrationController.add(null);
    }

    // Theme color from ESP
    if (message.startsWith("theme:")) {
      final hex = message.substring(6);
      _themeColorController.add(hex);
    }

    // BNO085 sensor status
    if (message == "bno:ok") {
      _bnoStatusController.add(true);
    } else if (message == "bno:error") {
      _bnoStatusController.add(false);
    }

    // Debug data from ESP
    if (message.startsWith("debug:")) {
      _debugController.add(message.substring(6));
    }

    // Calibration status from ESP
    if (message.startsWith("cal_status:")) {
      final statusStr = message.substring(11);
      final gears = ['R', '1', '2', '3', '4', '5', 'N'];
      final status = <String, bool>{};
      int expected = 0;
      for (int i = 0; i < gears.length && i < statusStr.length; i++) {
        final calibrated = statusStr[i] == '1';
        status[gears[i]] = calibrated;
        if (calibrated) expected++;
      }
      _calibrationStatusController.add(status);

      _pendingCalData.clear();
      _pendingCalExpected = expected;
      _calDataTimer?.cancel();

      if (expected > 0) {
        _calDataTimer = Timer(const Duration(seconds: 2), _flushCalData);
      }
    }

    // Calibration data from ESP
    if (message.startsWith("cal_data:")) {
      final payload = message.substring(9);
      final colonIndex = payload.indexOf(':');
      if (colonIndex > 0 && colonIndex < payload.length - 1) {
        final gear = payload.substring(0, colonIndex);
        final values = payload.substring(colonIndex + 1).split(',');
        if (values.length == 4) {
          try {
            final data = values.map((e) => double.parse(e)).toList();
            _pendingCalData[gear] = data;

            if (_pendingCalData.length >= _pendingCalExpected) {
              _calDataTimer?.cancel();
              _flushCalData();
            }
          } catch (_) {}
        }
      }
    }
  }

  void _flushCalData() {
    if (_pendingCalData.isNotEmpty) {
      _calibrationDataController.add(Map.from(_pendingCalData));
    }
    _pendingCalData.clear();
    _pendingCalExpected = 0;
  }

  //=====================================================
  // ENVIAR COLOR
  //=====================================================

  Future<void> sendColor(Color color) async {
    if (_commandCharacteristic == null) {
      debugPrint("No hay característica BLE");
      return;
    }

    final hex = "#${color.value.toRadixString(16).substring(2).toUpperCase()}";

    final command = "set_color:$hex";

    debugPrint("Enviando -> $command");

    await _commandCharacteristic!.write(
      command.codeUnits,
      withoutResponse: false,
    );
  }

  //=====================================================
  // STREAM BNO
  //=====================================================

  Future<void> startStream() async {
    if (_commandCharacteristic == null) return;

    await _commandCharacteristic!.write(
      "stream:on".codeUnits,
      withoutResponse: false,
    );
  }

  Future<void> stopStream() async {
    if (_commandCharacteristic == null) return;

    await _commandCharacteristic!.write(
      "stream:off".codeUnits,
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
  // CALIBRACIÓN
  //=====================================================

  Future<void> sendCalibration(String gear) async {
    if (_commandCharacteristic == null) return;

    await _commandCharacteristic!.write(
      "calibrate:$gear".codeUnits,
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

    _calDataTimer?.cancel();
    _pendingCalData.clear();
    _pendingCalExpected = 0;
    _bnoStatusController.add(false);
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
