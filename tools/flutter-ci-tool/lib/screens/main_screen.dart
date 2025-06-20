import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_tool_provider.dart';
import 'initiator_screen.dart';
import 'responder_screen.dart';
import 'log_screen.dart';
import 'settings_screen.dart';

class MainScreen extends StatefulWidget {
  const MainScreen({super.key});

  @override
  State<MainScreen> createState() => _MainScreenState();
}

class _MainScreenState extends State<MainScreen> with TickerProviderStateMixin {
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 4, vsync: this);
    
    WidgetsBinding.instance.addPostFrameCallback((_) {
      context.read<CIToolProvider>().initialize();
    });
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: const Text('MIDI-CI Tool'),
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(icon: Icon(Icons.send), text: 'Initiator'),
            Tab(icon: Icon(Icons.settings_input_antenna), text: 'Responder'),
            Tab(icon: Icon(Icons.list_alt), text: 'Log'),
            Tab(icon: Icon(Icons.settings), text: 'Settings'),
          ],
        ),
      ),
      body: TabBarView(
        controller: _tabController,
        children: const [
          InitiatorScreen(),
          ResponderScreen(),
          LogScreen(),
          SettingsScreen(),
        ],
      ),
    );
  }
}
