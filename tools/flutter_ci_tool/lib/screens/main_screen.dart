import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_tool_provider.dart';
import '../providers/ci_device_provider.dart';
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
      final toolProvider = context.read<CIToolProvider>();
      final deviceProvider = context.read<CIDeviceProvider>();
      
      // Set up log refresh callback from device provider to tool provider
      deviceProvider.setLogRefreshCallback(() {
        toolProvider.refreshLogs();
      });
      
      toolProvider.initialize();
    });
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CIToolProvider>(
      builder: (context, toolProvider, child) {
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
          body: Column(
            children: [
              // Library Status Banner
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(8.0),
                color: toolProvider.libraryLoaded 
                    ? Colors.green.shade100 
                    : Colors.red.shade100,
                child: Row(
                  children: [
                    Icon(
                      toolProvider.libraryLoaded ? Icons.check_circle : Icons.error,
                      color: toolProvider.libraryLoaded ? Colors.green : Colors.red,
                      size: 20,
                    ),
                    const SizedBox(width: 8),
                    Expanded(
                      child: Text(
                        toolProvider.libraryLoaded 
                            ? '✅ Native library loaded successfully'
                            : '❌ Native library failed to load: ${toolProvider.libraryLoadError ?? "Unknown error"}',
                        style: TextStyle(
                          color: toolProvider.libraryLoaded ? Colors.green.shade800 : Colors.red.shade800,
                          fontWeight: FontWeight.w500,
                        ),
                      ),
                    ),
                  ],
                ),
              ),
              
              // Main Content
              Expanded(
                child: TabBarView(
                  controller: _tabController,
                  children: const [
                    InitiatorScreen(),
                    ResponderScreen(),
                    LogScreen(),
                    SettingsScreen(),
                  ],
                ),
              ),
            ],
          ),
        );
      },
    );
  }
}