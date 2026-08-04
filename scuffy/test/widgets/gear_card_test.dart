import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/widgets/gear_card.dart';

void main() {
  Widget buildApp({required String gear, required bool calibrated}) {
    return MaterialApp(
      home: Scaffold(
        body: GearCard(
          gear: gear,
          calibrated: calibrated,
          onTap: () {},
        ),
      ),
    );
  }

  group('GearCard', () {
    testWidgets('displays gear name in title', (tester) async {
      await tester.pumpWidget(buildApp(gear: '3', calibrated: false));
      expect(find.text('Marcha 3'), findsOneWidget);
    });

    testWidgets('shows "Calibrada" when calibrated', (tester) async {
      await tester.pumpWidget(buildApp(gear: '1', calibrated: true));
      expect(find.text('Calibrada'), findsOneWidget);
      expect(find.text('Sin calibrar'), findsNothing);
    });

    testWidgets('shows "Sin calibrar" when not calibrated', (tester) async {
      await tester.pumpWidget(buildApp(gear: '2', calibrated: false));
      expect(find.text('Sin calibrar'), findsOneWidget);
      expect(find.text('Calibrada'), findsNothing);
    });

    testWidgets('displays the gear letter/number in the icon area', (tester) async {
      await tester.pumpWidget(buildApp(gear: 'R', calibrated: false));
      expect(find.text('R'), findsOneWidget);
    });

    testWidgets('calls onTap when tapped', (tester) async {
      bool tapped = false;
      await tester.pumpWidget(
        MaterialApp(
          home: Scaffold(
            body: GearCard(
              gear: 'N',
              calibrated: false,
              onTap: () => tapped = true,
            ),
          ),
        ),
      );
      await tester.tap(find.byType(GearCard));
      expect(tapped, true);
    });
  });
}
