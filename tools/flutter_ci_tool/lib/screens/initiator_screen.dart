import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_device_provider.dart';
import '../models/ci_device_model.dart';
import '../widgets/device_selector.dart';

class InitiatorScreen extends StatefulWidget {
  const InitiatorScreen({super.key});

  @override
  State<InitiatorScreen> createState() => _InitiatorScreenState();
}

class _InitiatorScreenState extends State<InitiatorScreen> {
  Connection? _selectedConnection;

  @override
  Widget build(BuildContext context) {
    return Consumer<CIDeviceProvider>(
      builder: (context, provider, child) {
        return Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Row(
                        children: [
                          const Icon(Icons.search, size: 24),
                          const SizedBox(width: 8),
                          Text(
                            'MIDI-CI Discovery',
                            style: Theme.of(context).textTheme.titleLarge,
                          ),
                        ],
                      ),
                      const SizedBox(height: 16),
                      const Text(
                        'Send MIDI-CI discovery messages to find remote devices. '
                        'This calls the C++ CIDeviceModel::send_discovery() method.',
                      ),
                      const SizedBox(height: 16),
                      Row(
                        children: [
                          ElevatedButton.icon(
                            onPressed: () => provider.sendDiscovery(),
                            icon: const Icon(Icons.send),
                            label: const Text('Send Discovery'),
                          ),
                          const SizedBox(width: 16),
                          ElevatedButton.icon(
                            onPressed: () => provider.refreshConnections(),
                            icon: const Icon(Icons.refresh),
                            label: const Text('Refresh'),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              
              const SizedBox(height: 16),
              
              const DeviceSelector(),
              
              const SizedBox(height: 16),
              
              Expanded(
                child: Card(
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            const Icon(Icons.devices, size: 24),
                            const SizedBox(width: 8),
                            Text(
                              'Remote Connections',
                              style: Theme.of(context).textTheme.titleLarge,
                            ),
                            const Spacer(),
                            Text(
                              '${provider.connections.length} connected',
                              style: Theme.of(context).textTheme.bodyMedium,
                            ),
                          ],
                        ),
                        const SizedBox(height: 16),
                        
                        if (provider.connections.isEmpty)
                          const Expanded(
                            child: Center(
                              child: Column(
                                mainAxisAlignment: MainAxisAlignment.center,
                                children: [
                                  Icon(
                                    Icons.device_unknown,
                                    size: 64,
                                    color: Colors.grey,
                                  ),
                                  SizedBox(height: 16),
                                  Text(
                                    'No remote devices found',
                                    style: TextStyle(
                                      fontSize: 18,
                                      color: Colors.grey,
                                    ),
                                  ),
                                  SizedBox(height: 8),
                                  Text(
                                    'Send discovery to find MIDI-CI devices',
                                    style: TextStyle(color: Colors.grey),
                                  ),
                                ],
                              ),
                            ),
                          )
                        else
                          Expanded(
                            child: ListView.builder(
                              itemCount: provider.connections.length,
                              itemBuilder: (context, index) {
                                final connection = provider.connections[index];
                                return ConnectionCard(
                                  connection: connection,
                                  isSelected: _selectedConnection == connection,
                                  onTap: () {
                                    setState(() {
                                      _selectedConnection = connection;
                                    });
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
              
              if (provider.lastError != null)
                Container(
                  margin: const EdgeInsets.only(top: 16),
                  padding: const EdgeInsets.all(12),
                  decoration: BoxDecoration(
                    color: Colors.red.shade100,
                    borderRadius: BorderRadius.circular(8),
                    border: Border.all(color: Colors.red.shade300),
                  ),
                  child: Row(
                    children: [
                      Icon(Icons.error, color: Colors.red.shade700),
                      const SizedBox(width: 8),
                      Expanded(
                        child: Text(
                          provider.lastError!,
                          style: TextStyle(color: Colors.red.shade700),
                        ),
                      ),
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

class ConnectionCard extends StatelessWidget {
  final Connection connection;
  final bool isSelected;
  final VoidCallback onTap;

  const ConnectionCard({
    super.key,
    required this.connection,
    required this.isSelected,
    required this.onTap,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      elevation: isSelected ? 4 : 1,
      color: isSelected ? Theme.of(context).colorScheme.primaryContainer : null,
      child: InkWell(
        onTap: onTap,
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Row(
                children: [
                  Icon(
                    connection.isConnected ? Icons.link : Icons.link_off,
                    color: connection.isConnected ? Colors.green : Colors.grey,
                  ),
                  const SizedBox(width: 8),
                  Text(
                    'MUID: ${connection.targetMuid.toRadixString(16).toUpperCase()}',
                    style: Theme.of(context).textTheme.titleMedium?.copyWith(
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const Spacer(),
                  if (isSelected)
                    Icon(
                      Icons.check_circle,
                      color: Theme.of(context).colorScheme.primary,
                    ),
                ],
              ),
              const SizedBox(height: 8),
              Text(
                connection.deviceInfo,
                style: Theme.of(context).textTheme.bodyMedium,
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  _buildInfoChip(
                    context,
                    'Profiles: ${connection.profiles.length}',
                    Icons.account_tree,
                  ),
                  const SizedBox(width: 8),
                  _buildInfoChip(
                    context,
                    'Properties: ${connection.properties.length}',
                    Icons.settings,
                  ),
                ],
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildInfoChip(BuildContext context, String label, IconData icon) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
      decoration: BoxDecoration(
        color: Theme.of(context).colorScheme.secondaryContainer,
        borderRadius: BorderRadius.circular(12),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 16),
          const SizedBox(width: 4),
          Text(
            label,
            style: Theme.of(context).textTheme.bodySmall,
          ),
        ],
      ),
    );
  }
}
