import 'package:flutter/material.dart';
import 'package:flutter_test/flutter_test.dart';
import 'package:scuffy/widgets/status_card.dart';

void main() {
  Widget buildApp(bool connected) {
    return MaterialApp(
      home: Scaffold(body: StatusCard(connected: connected)),
    );
  }

  group('StatusCard', () {
    testWidgets('shows "Conectado" when connected', (tester) async {
      await tester.pumpWidget(buildApp(true));
      expect(find.text('Conectado'), findsOneWidget);
      expect(find.text('Desconectado'), findsNothing);
    });

    testWidgets('shows "Desconectado" when not connected', (tester) async {
      await tester.pumpWidget(buildApp(false));
      expect(find.text('Desconectado'), findsOneWidget);
      expect(find.text('Conectado'), findsNothing);
    });

    testWidgets('renders without crashing', (tester) async {
      await tester.pumpWidget(buildApp(true));
      expect(find.byType(StatusCard), findsOneWidget);
    });
  });
}
