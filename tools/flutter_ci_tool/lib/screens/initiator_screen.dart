import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_tool_provider.dart';
import '../providers/ci_device_provider.dart';

class InitiatorScreen extends StatelessWidget {
  const InitiatorScreen({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer2<CIToolProvider, CIDeviceProvider>(
      builder: (context, toolProvider, deviceProvider, child) {
        return Padding(
          padding: const EdgeInsets.all(16.0),
          child: SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
              // Status Card
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Status',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 8),
                      Row(
                        children: [
                          Icon(
                            toolProvider.isInitialized
                                ? Icons.check_circle
                                : Icons.error,
                            color: toolProvider.isInitialized
                                ? Colors.green
                                : Colors.red,
                          ),
                          const SizedBox(width: 8),
                          Text(
                            toolProvider.isInitialized
                                ? 'Initialized'
                                : 'Not Initialized',
                          ),
                        ],
                      ),
                      if (toolProvider.lastError != null) ...[
                        const SizedBox(height: 8),
                        Text(
                          'Error: ${toolProvider.lastError}',
                          style: const TextStyle(color: Colors.red),
                        ),
                      ],
                      if (toolProvider.isInitialized) ...[
                        const SizedBox(height: 8),
                        Text('MUID: 0x${toolProvider.getMuid().toRadixString(16).toUpperCase()}'),
                      ],
                    ],
                  ),
                ),
              ),
              
              const SizedBox(height: 16),
              
              // Discovery Card
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        'Device Discovery',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const SizedBox(height: 16),
                      ElevatedButton.icon(
                        onPressed: toolProvider.isInitialized
                            ? () => deviceProvider.sendDiscovery()
                            : null,
                        icon: const Icon(Icons.search),
                        label: const Text('Send Discovery'),
                      ),
                      const SizedBox(height: 16),
                      Text('Connections: ${deviceProvider.connections.length}'),
                    ],
                  ),
                ),
              ),
              
              const SizedBox(height: 16),
              
              // Connections List
              SizedBox(
                height: 300,
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Text(
                          'Remote Devices',
                          style: Theme.of(context).textTheme.titleLarge,
                        ),
                        const SizedBox(height: 16),
                        Expanded(
                          child: ValueListenableBuilder(
                            valueListenable: deviceProvider.connections,
                            builder: (context, connections, child) {
                              return connections.isEmpty
                                  ? const Center(
                                      child: Text('No devices found. Try sending discovery.'),
                                    )
                                  : ListView.builder(
                                      itemCount: connections.length,
                                      itemBuilder: (context, index) {
                                        final connectionModel = connections[index];
                                        final connection = connectionModel.connection;
                                        return ListTile(
                                          leading: const Icon(Icons.device_hub),
                                          title: Text('Device ${connection.targetMuid}'),
                                          subtitle: Text('MUID: 0x${connection.targetMuid.toRadixString(16).toUpperCase()}'),
                                        );
                                      },
                                    );
                            },
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ),
              ],
            ),
          ),
        );
      },
    );
  }

}