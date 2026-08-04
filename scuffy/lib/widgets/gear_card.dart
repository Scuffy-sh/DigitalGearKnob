import 'package:flutter/material.dart';

class GearCard extends StatelessWidget {
  final String gear;
  final bool calibrated;
  final Color accentColor;
  final VoidCallback onTap;

  const GearCard({
    super.key,
    required this.gear,
    required this.calibrated,
    required this.accentColor,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Container(
      decoration: BoxDecoration(
        gradient: LinearGradient(
          begin: Alignment.topLeft,
          end: Alignment.bottomRight,
          colors: [
            Colors.white.withValues(alpha: 0.08),
            Colors.white.withValues(alpha: 0.03),
          ],
        ),
        borderRadius: BorderRadius.circular(18),
        border: Border.all(
            color: calibrated
                ? accentColor.withValues(alpha: 0.3)
                : Colors.white.withValues(alpha: 0.1),
          width: 1,
        ),
        boxShadow: calibrated
            ? [
                BoxShadow(
                  color: accentColor.withValues(alpha: 0.12),
                  blurRadius: 12,
                  spreadRadius: -2,
                ),
              ]
            : [
                BoxShadow(
                  color: Colors.black.withValues(alpha: 0.2),
                  blurRadius: 6,
                  offset: const Offset(0, 2),
                ),
              ],
      ),
      child: Material(
        color: Colors.transparent,
        borderRadius: BorderRadius.circular(18),
        child: InkWell(
          borderRadius: BorderRadius.circular(18),
          onTap: onTap,
          child: Padding(
            padding: const EdgeInsets.symmetric(horizontal: 20, vertical: 18),
            child: Row(
              children: [
                Container(
                  width: 52,
                  height: 52,
                  decoration: BoxDecoration(
                    color: calibrated
                        ? accentColor.withValues(alpha: 0.15)
                        : Colors.grey.withValues(alpha: 0.15),
                    borderRadius: BorderRadius.circular(14),
                  ),
                  child: Center(
                    child: Text(
                      gear,
                      style: TextStyle(
                        fontSize: 26,
                        fontWeight: FontWeight.bold,
                        color: calibrated ? accentColor : Colors.white,
                      ),
                    ),
                  ),
                ),

                const SizedBox(width: 20),

                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        "Marcha $gear",
                        style: const TextStyle(
                          fontSize: 18,
                          fontWeight: FontWeight.w600,
                        ),
                      ),

                      const SizedBox(height: 4),

                      Text(
                        calibrated ? "Calibrada" : "Sin calibrar",
                        style: TextStyle(
                          color: calibrated ? accentColor : Colors.grey,
                        ),
                      ),
                    ],
                  ),
                ),

                const Icon(Icons.chevron_right, size: 32, color: Colors.white54),
              ],
            ),
          ),
        ),
      ),
    );
  }
}
