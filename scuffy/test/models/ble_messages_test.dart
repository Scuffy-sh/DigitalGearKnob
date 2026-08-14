import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/ble_messages.dart';

void main() {
  group('parseCanDebug', () {
    test('parses a full debug line with CAN online', () {
      final result = parseCanDebug('debug:3,2450,52,ok');

      expect(result, isNotNull);
      expect(result!.gear, '3');
      expect(result.rpm, 2450);
      expect(result.speed, 52);
      expect(result.canOnline, isTrue);
    });

    test('parses reverse gear with CAN offline', () {
      final result = parseCanDebug('debug:R,800,0,offline');

      expect(result, isNotNull);
      expect(result!.gear, 'R');
      expect(result.rpm, 800);
      expect(result.speed, 0);
      expect(result.canOnline, isFalse);
    });

    test('parses unknown gear placeholder', () {
      final result = parseCanDebug('debug:??,1200,30,ok');

      expect(result, isNotNull);
      expect(result!.gear, '??');
      expect(result.rpm, 1200);
      expect(result.speed, 30);
    });

    test('returns null for a non-debug message', () {
      expect(parseCanDebug('theme:#0052FF'), isNull);
    });

    test('returns null for a malformed payload', () {
      expect(parseCanDebug('debug:3,2450'), isNull);
      expect(parseCanDebug('debug:3,abc,52,ok'), isNull);
      expect(parseCanDebug('debug:3,2450,52,maybe'), isNull);
    });
  });

  group('parseSniffFrame', () {
    test('parses a standard frame', () {
      final result = parseSniffFrame('sniff:S00000280:0A1B');

      expect(result, isNotNull);
      expect(result!.extended, isFalse);
      expect(result.idHex, '00000280');
      expect(result.dataHex, '0A1B');
    });

    test('parses an extended frame', () {
      final result = parseSniffFrame('sniff:E018DA00F:01');

      expect(result, isNotNull);
      expect(result!.extended, isTrue);
      expect(result.idHex, '018DA00F');
      expect(result.dataHex, '01');
    });

    test('parses an empty payload', () {
      final result = parseSniffFrame('sniff:S00000280:');

      expect(result, isNotNull);
      expect(result!.dataHex, isEmpty);
    });

    test('returns null for a non-sniff message', () {
      expect(parseSniffFrame('can:no_frames'), isNull);
      expect(parseSniffFrame('debug:3,2450,52,ok'), isNull);
    });

    test('returns null for malformed frames', () {
      expect(parseSniffFrame('sniff:'), isNull);
      expect(parseSniffFrame('sniff:S0000028'), isNull);
      expect(parseSniffFrame('sniff:X00000280:0A'), isNull);
    });
  });
}
