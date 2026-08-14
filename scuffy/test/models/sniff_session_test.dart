import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/sniff_session.dart';

void main() {
  group('SniffSession', () {
    test('starts disabled with no state', () {
      final session = SniffSession.initial();

      expect(session.enabled, isFalse);
      expect(session.frames, isEmpty);
      expect(session.canOnline, isNull);
      expect(session.protocolVersion, isNull);
      expect(session.busSilent, isFalse);
    });

    test('toggle flips enabled state', () {
      final on = SniffSession.initial().toggle();
      expect(on.enabled, isTrue);
      expect(on.toggle().enabled, isFalse);
    });

    test('collects sniff frames preserving order', () {
      final session = SniffSession.initial()
          .handleMessage('sniff:S00000280:0A')
          .handleMessage('sniff:S00000320:0B')
          .handleMessage('sniff:S00000280:0C');

      expect(session.frames, [
        'sniff:S00000280:0A',
        'sniff:S00000320:0B',
        'sniff:S00000280:0C',
      ]);
      expect(session.busSilent, isFalse);
    });

    test('ignores non-sniff messages for the frame list', () {
      final session = SniffSession.initial()
          .handleMessage('debug:3,2450,52,ok')
          .handleMessage('theme:#0052FF');

      expect(session.frames, isEmpty);
    });

    test('flags a silent bus on can:no_frames', () {
      final session = SniffSession.initial().handleMessage('can:no_frames');

      expect(session.busSilent, isTrue);
    });

    test('tracks CAN status', () {
      final online = SniffSession.initial().handleMessage('can:ok');
      expect(online.canOnline, isTrue);
      expect(online.busSilent, isFalse);

      final offline = online.handleMessage('can:offline');
      expect(offline.canOnline, isFalse);
    });

    test('tracks protocol version', () {
      final session = SniffSession.initial().handleMessage('proto:2');
      expect(session.protocolVersion, 2);
    });

    test('caps the frame list to the newest frames', () {
      var session = SniffSession.initial();
      for (int i = 0; i < SniffSession.maxFrames + 50; i++) {
        session = session.handleMessage('sniff:S00000280:${i.toRadixString(16)}');
      }

      expect(session.frames.length, SniffSession.maxFrames);
      expect(session.frames.first, 'sniff:S00000280:${(50).toRadixString(16)}');
    });

    test('clearFrames resets the list and silent flag', () {
      final session = SniffSession.initial()
          .handleMessage('sniff:S00000280:0A')
          .handleMessage('can:no_frames')
          .clearFrames();

      expect(session.frames, isEmpty);
      expect(session.busSilent, isFalse);
    });
  });
}
