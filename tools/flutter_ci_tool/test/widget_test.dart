import 'package:flutter_test/flutter_test.dart';
import 'package:midicci_flutter_gui/main.dart';

void main() {
  testWidgets('App smoke test', (WidgetTester tester) async {
    // Build our app and trigger a frame.
    await tester.pumpWidget(const MidiCIApp());

    // Verify that our app starts up correctly
    expect(find.text('MIDI-CI Tool'), findsOneWidget);
  });
}