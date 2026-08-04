import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/calibration_state.dart';

void main() {
  group('CalibrationStateExtension.title', () {
    test('returns correct title for waiting', () {
      expect(CalibrationState.waiting.title, 'Esperando calibración');
    });

    test('returns correct title for receiving', () {
      expect(CalibrationState.receiving.title, 'Recibiendo datos');
    });

    test('returns correct title for saving', () {
      expect(CalibrationState.saving.title, 'Guardando posición');
    });

    test('returns correct title for completed', () {
      expect(CalibrationState.completed.title, 'Calibración completada');
    });

    test('every state has a non-empty title', () {
      for (final state in CalibrationState.values) {
        expect(state.title.isNotEmpty, true, reason: 'title for $state should not be empty');
      }
    });
  });
}
