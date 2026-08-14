import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/widgets/gear_card.dart';

void main() {
  Widget buildApp({required String gear}) {
    return MaterialApp(
      home: Scaffold(
        body: GearCard(
          gear: gear,
          accentColor: const Color(0xFF0052FF),
        ),
      ),
    );
  }

  group('GearCard', () {
    testWidgets('displays gear name in title', (tester) async {
      await tester.pumpWidget(buildApp(gear: '3'));
      expect(find.text('Marcha 3'), findsOneWidget);
    });

    testWidgets('displays the gear letter/number in the icon area',
        (tester) async {
      await tester.pumpWidget(buildApp(gear: 'R'));
      expect(find.text('R'), findsOneWidget);
    });

    testWidgets('does not show calibration state', (tester) async {
      await tester.pumpWidget(buildApp(gear: '2'));
      expect(find.text('Calibrada'), findsNothing);
      expect(find.text('Sin calibrar'), findsNothing);
    });

    testWidgets('has no navigation affordance', (tester) async {
      await tester.pumpWidget(buildApp(gear: 'N'));
      expect(find.byIcon(Icons.chevron_right), findsNothing);
    });
  });
}
