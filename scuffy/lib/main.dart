import 'package:flutter/material.dart';
import 'screens/splash_screen.dart';

void main() {
  runApp(const ScuffyApp());
}

class ScuffyApp extends StatelessWidget {
  const ScuffyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      debugShowCheckedModeBanner: false,
      title: 'Scuffy',
      theme: ThemeData.dark(),
      home: const SplashScreen(),
    );
  }
}
