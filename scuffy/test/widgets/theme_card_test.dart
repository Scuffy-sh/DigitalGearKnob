import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/models/theme_model.dart';
import 'package:scuffy/widgets/theme_card.dart';

void main() {
  Widget buildApp({required bool selected}) {
    const theme = ScuffyTheme(name: 'Azul', color: Colors.blue);
    return MaterialApp(
      home: Scaffold(
        body: ThemeCard(
          theme: theme,
          selected: selected,
          onTap: () {},
        ),
      ),
    );
  }

  group('ThemeCard', () {
    testWidgets('displays theme name', (tester) async {
      await tester.pumpWidget(buildApp(selected: false));
      expect(find.text('Azul'), findsOneWidget);
    });

    testWidgets('shows check icon when selected', (tester) async {
      await tester.pumpWidget(buildApp(selected: true));
      expect(find.byIcon(Icons.check_circle), findsOneWidget);
    });

    testWidgets('hides check icon when not selected', (tester) async {
      await tester.pumpWidget(buildApp(selected: false));
      expect(find.byIcon(Icons.check_circle), findsNothing);
    });

    testWidgets('calls onTap when tapped', (tester) async {
      bool tapped = false;
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: ThemeCard(
              theme: const ScuffyTheme(name: 'Test', color: Colors.red),
              selected: false,
              onTap: () => tapped = true,
            ),
          ),
        ),
      );
      await tester.tap(find.byType(ThemeCard));
      expect(tapped, true);
    });
  });
}
