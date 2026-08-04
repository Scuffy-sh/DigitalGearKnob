enum CalibrationState { waiting, receiving, saving, completed }

extension CalibrationStateExtension on CalibrationState {
  String get title {
    switch (this) {
      case CalibrationState.waiting:
        return "Esperando calibración";

      case CalibrationState.receiving:
        return "Recibiendo datos";

      case CalibrationState.saving:
        return "Guardando posición";

      case CalibrationState.completed:
        return "Calibración completada";
    }
  }
}
