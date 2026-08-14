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

  static Future<void> clearAll() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_keyThemeColor);
  }
}
