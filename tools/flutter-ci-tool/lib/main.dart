import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'providers/ci_tool_provider.dart';
import 'providers/ci_device_provider.dart';
import 'screens/main_screen.dart';

void main() {
  runApp(const MidiCIApp());
}

class MidiCIApp extends StatelessWidget {
  const MidiCIApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MultiProvider(
      providers: [
        ChangeNotifierProvider(create: (_) => CIToolProvider()),
        ChangeNotifierProvider(create: (_) => CIDeviceProvider()),
      ],
      child: MaterialApp(
        title: 'MIDI-CI Tool',
        theme: ThemeData(
          colorScheme: ColorScheme.fromSeed(seedColor: Colors.deepPurple),
          useMaterial3: true,
        ),
        home: const MainScreen(),
      ),
    );
  }
}
