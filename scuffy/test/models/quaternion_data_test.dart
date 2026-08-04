import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/quaternion_data.dart';

void main() {
  group('QuaternionData', () {
    test('stores all four components', () {
      const q = QuaternionData(w: 1.0, x: 2.0, y: 3.0, z: 4.0);
      expect(q.w, 1.0);
      expect(q.x, 2.0);
      expect(q.y, 3.0);
      expect(q.z, 4.0);
    });

    test('is a const constructor', () {
      const q = QuaternionData(w: 0, x: 0, y: 0, z: 0);
      expect(q.w, 0);
    });

    test('holds double precision values', () {
      const q = QuaternionData(w: 0.123456789, x: 0.987654321, y: 1.1, z: 2.2);
      expect(q.w, closeTo(0.123456789, 1e-9));
      expect(q.x, closeTo(0.987654321, 1e-9));
    });
  });
}
