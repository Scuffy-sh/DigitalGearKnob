import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/screens/sniff_screen.dart';
import 'package:scuffy/services/ble_service.dart';

void main() {
  Widget buildApp() {
    return MaterialApp(
      home: SniffScreen(
        bluetooth: BleService(),
        accentColor: const Color(0xFF0052FF),
      ),
    );
  }

  group('SniffScreen', () {
    testWidgets('renders with sniff off and no frames', (tester) async {
      await tester.pumpWidget(buildApp());

      expect(find.text('Sniff CAN'), findsOneWidget);
      expect(find.text('SNIFF: OFF'), findsOneWidget);
      expect(find.text('Sin tramas capturadas.\nActivá SNIFF para ver el bus CAN.'),
          findsOneWidget);
    });

    testWidgets('tapping the toggle flips the label to ON', (tester) async {
      await tester.pumpWidget(buildApp());

      await tester.tap(find.text('SNIFF: OFF'));
      await tester.pump();

      expect(find.text('SNIFF: ON'), findsOneWidget);
      expect(find.text('SNIFF: OFF'), findsNothing);
    });

    testWidgets('toggling twice returns to OFF', (tester) async {
      await tester.pumpWidget(buildApp());

      await tester.tap(find.text('SNIFF: OFF'));
      await tester.pump();
      await tester.tap(find.text('SNIFF: ON'));
      await tester.pump();

      expect(find.text('SNIFF: OFF'), findsOneWidget);
    });

    testWidgets('clear button is disabled when there are no frames',
        (tester) async {
      await tester.pumpWidget(buildApp());

      final clearButton = tester.widget<IconButton>(
        find.ancestor(
          of: find.byIcon(Icons.delete_sweep),
          matching: find.byType(IconButton),
        ),
      );
      expect(clearButton.onPressed, isNull);
    });
  });
}
