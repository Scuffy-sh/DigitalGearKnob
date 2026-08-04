import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/gear.dart';

void main() {
  group('GearExtension.name', () {
    test('returns correct name for all gear values', () {
      expect(Gear.reverse.name, 'R');
      expect(Gear.first.name, '1');
      expect(Gear.second.name, '2');
      expect(Gear.third.name, '3');
      expect(Gear.fourth.name, '4');
      expect(Gear.fifth.name, '5');
      expect(Gear.neutral.name, 'N');
    });

    test('returns short string for every gear', () {
      for (final gear in Gear.values) {
        expect(gear.name.length, 1);
      }
    });
  });

  group('GearInfo', () {
    test('default values are calibrated=false and w=1', () {
      final info = GearInfo(gear: Gear.first);
      expect(info.gear, Gear.first);
      expect(info.calibrated, false);
      expect(info.w, 1.0);
      expect(info.x, 0.0);
      expect(info.y, 0.0);
      expect(info.z, 0.0);
    });

    test('accepts custom quaternion values', () {
      final info = GearInfo(
        gear: Gear.third,
        calibrated: true,
        w: 0.5,
        x: 0.1,
        y: 0.2,
        z: 0.3,
      );
      expect(info.gear, Gear.third);
      expect(info.calibrated, true);
      expect(info.w, 0.5);
      expect(info.x, 0.1);
      expect(info.y, 0.2);
      expect(info.z, 0.3);
    });

    test('calibrated is mutable', () {
      final info = GearInfo(gear: Gear.neutral);
      expect(info.calibrated, false);
      info.calibrated = true;
      expect(info.calibrated, true);
    });

    test('quaternion fields are mutable', () {
      final info = GearInfo(gear: Gear.reverse);
      info.w = 9.0;
      info.x = 8.0;
      info.y = 7.0;
      info.z = 6.0;
      expect(info.w, 9.0);
      expect(info.x, 8.0);
      expect(info.y, 7.0);
      expect(info.z, 6.0);
    });
  });
}
