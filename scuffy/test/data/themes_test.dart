import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/data/themes.dart';

void main() {
  group('scuffyThemes', () {
    test('contains exactly 5 themes', () {
      expect(scuffyThemes.length, 5);
    });

    test('last theme is the custom one', () {
      expect(scuffyThemes.last.custom, true);
      expect(scuffyThemes.last.name, contains('Personalizado'));
    });

    test('first four themes are not custom', () {
      for (int i = 0; i < 4; i++) {
        expect(scuffyThemes[i].custom, false, reason: 'theme at index $i should not be custom');
      }
    });

    test('all themes have non-empty names', () {
      for (final theme in scuffyThemes) {
        expect(theme.name.isNotEmpty, true);
      }
    });
  });
}
