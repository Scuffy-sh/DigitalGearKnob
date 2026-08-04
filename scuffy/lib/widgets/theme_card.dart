import 'package:flutter/material.dart';
import '../models/theme_model.dart';

class ThemeCard extends StatelessWidget {
  final ScuffyTheme theme;
  final bool selected;
  final VoidCallback onTap;

  const ThemeCard({
    super.key,
    required this.theme,
    required this.selected,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: onTap,

      child: AnimatedContainer(
        duration: const Duration(milliseconds: 250),

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
          borderRadius: BorderRadius.circular(18),
          border: Border.all(
            color: selected
                ? theme.color.withValues(alpha: 0.5)
                : Colors.white.withValues(alpha: 0.1),
            width: selected ? 2 : 1,
          ),
          boxShadow: selected
              ? [
                  BoxShadow(
                    color: theme.color.withValues(alpha: 0.15),
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

        child: Row(
          children: [
            CircleAvatar(radius: 18, backgroundColor: theme.color),

            const SizedBox(width: 20),

            Expanded(
              child: Text(
                theme.name,

                style: const TextStyle(
                  fontSize: 18,
                  fontWeight: FontWeight.w600,
                ),
              ),
            ),

            if (selected)
              Icon(Icons.check_circle, color: theme.color, size: 30),
          ],
        ),
      ),
    );
  }
}
