import 'package:flutter/material.dart';

class StatusCard extends StatelessWidget {
  final bool connected;
  final Color? accentColor;

  const StatusCard({super.key, required this.connected, this.accentColor});

  @override
  Widget build(BuildContext context) {
    final Color indicatorColor =
        connected ? (accentColor ?? Colors.greenAccent) : Colors.redAccent;

    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(18),
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            Colors.white.withValues(alpha: 0.08),
            Colors.white.withValues(alpha: 0.03),
          ],
        ),
        borderRadius: BorderRadius.circular(20),
        border: Border.all(
          color: Colors.white.withValues(alpha: 0.1),
          width: 1,
        ),
        boxShadow: connected
            ? [
                BoxShadow(
                  color: indicatorColor.withValues(alpha: 0.2),
                  blurRadius: 16,
                  spreadRadius: -2,
                ),
              ]
            : [
                BoxShadow(
                  color: Colors.black.withValues(alpha: 0.3),
                  blurRadius: 8,
                  offset: const Offset(0, 2),
                ),
              ],
      ),
      child: Row(
        children: [
          Icon(
            Icons.circle,
            color: indicatorColor,
            size: 18,
          ),
          const SizedBox(width: 12),
          Text(
            connected ? "Conectado" : "Desconectado",
            style: const TextStyle(fontSize: 18, fontWeight: FontWeight.w600),
          ),
        ],
      ),
    );
  }
}
