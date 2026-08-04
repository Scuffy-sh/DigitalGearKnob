import 'package:shared_preferences/shared_preferences.dart';

class StateStorage {
  static const _keyThemeColor = 'theme_color';

  static Future<void> saveThemeColor(String hexColor) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_keyThemeColor, hexColor);
  }

  static Future<String?> loadThemeColor() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_keyThemeColor);
  }

  static Future<void> saveCalibrationStatus(Map<String, bool> status) async {
    final prefs = await SharedPreferences.getInstance();
    for (final entry in status.entries) {
      await prefs.setBool('cal_${entry.key}', entry.value);
    }
  }

  static Future<Map<String, bool>> loadCalibrationStatus() async {
    final prefs = await SharedPreferences.getInstance();
    final gears = ['R', '1', '2', '3', '4', '5', 'N'];
    final result = <String, bool>{};
    for (final g in gears) {
      result[g] = prefs.getBool('cal_$g') ?? false;
    }
    return result;
  }

  static Future<void> saveCalibrationData(
      String gear, double w, double x, double y, double z) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setString('cal_data_$gear', '$w,$x,$y,$z');
  }

  static Future<Map<String, List<double>>?> loadCalibrationData() async {
    final prefs = await SharedPreferences.getInstance();
    final gears = ['R', '1', '2', '3', '4', '5', 'N'];
    final result = <String, List<double>>{};
    bool anyFound = false;

    for (final g in gears) {
      final str = prefs.getString('cal_data_$g');
      if (str != null) {
        final parts = str.split(',');
        if (parts.length == 4) {
          result[g] = parts.map((e) => double.tryParse(e) ?? 0.0).toList();
          anyFound = true;
        }
      }
    }

    return anyFound ? result : null;
  }

  static Future<void> clearAll() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_keyThemeColor);
    final gears = ['R', '1', '2', '3', '4', '5', 'N'];
    for (final g in gears) {
      await prefs.remove('cal_$g');
      await prefs.remove('cal_data_$g');
    }
  }
}
