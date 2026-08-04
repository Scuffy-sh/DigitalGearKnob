import 'package:flutter/material.dart';

class ScuffyTheme {
  final String name;
  final Color color;

  /// Indica si es la tarjeta de color personalizado
  final bool custom;

  const ScuffyTheme({
    required this.name,
    required this.color,
    this.custom = false,
  });
}
