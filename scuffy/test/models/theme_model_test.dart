import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/theme_model.dart';

void main() {
  group('ScuffyTheme', () {
    test('stores name and color', () {
      const theme = ScuffyTheme(name: 'Test', color: Colors.blue);
      expect(theme.name, 'Test');
      expect(theme.color, Colors.blue);
    });

    test('custom defaults to false', () {
      const theme = ScuffyTheme(name: 'A', color: Colors.red);
      expect(theme.custom, false);
    });

    test('custom can be set to true', () {
      const theme = ScuffyTheme(name: 'B', color: Colors.green, custom: true);
      expect(theme.custom, true);
    });

    test('is a const constructor', () {
      const theme = ScuffyTheme(name: 'C', color: Colors.white);
      expect(theme.name, 'C');
    });
  });
}
