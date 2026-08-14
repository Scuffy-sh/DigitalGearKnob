// Parsed v2 BLE payloads from the knob (see design "Protocolo BLE v2"
// table). Pure parsing functions, unit-tested without any BLE stack.

/// Debug notification payload: `debug:<GEAR>,<RPM>,<SPEED>,<CANSTAT>`.
class CanDebugData {
  final String gear;
  final int rpm;
  final int speed;
  final bool canOnline;

  const CanDebugData({
    required this.gear,
    required this.rpm,
    required this.speed,
    required this.canOnline,
  });
}

/// Parses a full `debug:...` notification line.
///
/// Returns null when the message is not a debug line or its payload is
/// malformed (wrong field count, non-numeric rpm/speed, unknown CANSTAT).
CanDebugData? parseCanDebug(String message) {
  if (!message.startsWith('debug:')) return null;

  final parts = message.substring(6).split(',');
  if (parts.length != 4) return null;

  final rpm = int.tryParse(parts[1]);
  final speed = int.tryParse(parts[2]);
  if (rpm == null || speed == null) return null;

  final canStat = parts[3];
  if (canStat != 'ok' && canStat != 'offline') return null;

  return CanDebugData(
    gear: parts[0],
    rpm: rpm,
    speed: speed,
    canOnline: canStat == 'ok',
  );
}

/// Sniff notification: `sniff:<E|S><8hexid>:<datahex>`.
class SniffFrame {
  final bool extended;
  final String idHex;
  final String dataHex;

  const SniffFrame({
    required this.extended,
    required this.idHex,
    required this.dataHex,
  });
}

bool _isHexDigit(String s) =>
    s.length == 1 && int.tryParse(s, radix: 16) != null;

/// Parses a full `sniff:...` notification line (the knob always emits
/// 8 hex chars for the ID, standard frames zero-padded).
///
/// Returns null when the message is not a sniff line or its payload is
/// malformed.
SniffFrame? parseSniffFrame(String message) {
  if (!message.startsWith('sniff:')) return null;

  final rest = message.substring(6);
  if (rest.length < 10) return null;

  final type = rest[0];
  if (type != 'E' && type != 'S') return null;

  final idHex = rest.substring(1, 9);
  if (idHex.split('').any((c) => !_isHexDigit(c))) return null;

  if (rest[9] != ':') return null;

  return SniffFrame(
    extended: type == 'E',
    idHex: idHex,
    dataHex: rest.substring(10),
  );
}
