import 'package:flutter/foundation.dart';
import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_tool_provider.dart';
import '../providers/ci_device_provider.dart';

class SettingsScreen extends StatefulWidget {
  const SettingsScreen({super.key});

  @override
  State<SettingsScreen> createState() => _SettingsScreenState();
}

class _SettingsScreenState extends State<SettingsScreen> {
  final TextEditingController _configPathController = TextEditingController();

  @override
  void initState() {
    super.initState();
    // Refresh MIDI devices when the screen loads
    WidgetsBinding.instance.addPostFrameCallback((_) {
      if (mounted) {
        try {
          context.read<CIDeviceProvider>().refreshDevices();
        } catch (e) {
          // Handle potential context issues gracefully
          if (kDebugMode) {
            debugPrint('❌ Error refreshing devices in Settings: $e');
          }
        }
      }
    });
  }

  @override
  void dispose() {
    _configPathController.dispose();
    super.dispose();
  }

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
              _buildConfigurationSection(toolProvider),
              const SizedBox(height: 24),
              _buildDeviceSection(deviceProvider),
              const SizedBox(height: 24),
              _buildApplicationSection(toolProvider),
              if (toolProvider.lastError != null || deviceProvider.lastError.get() != null)
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
                          toolProvider.lastError ?? deviceProvider.lastError.get() ?? '',
                          style: TextStyle(color: Colors.red.shade700),
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
          ),
        );
      },
    );
  }

  Widget _buildConfigurationSection(CIToolProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.settings, size: 24),
                const SizedBox(width: 8),
                Text(
                  'Configuration',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
              ],
            ),
            const SizedBox(height: 16),
            Text(
              'Current config: ${provider.configPath.isEmpty ? 'default' : provider.configPath}',
              style: Theme.of(context).textTheme.bodyMedium,
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: _configPathController,
                    decoration: const InputDecoration(
                      labelText: 'Configuration File Path',
                      hintText: '/path/to/config.json',
                      border: OutlineInputBorder(),
                    ),
                  ),
                ),
                const SizedBox(width: 16),
                ElevatedButton(
                  onPressed: () {
                    if (_configPathController.text.isNotEmpty) {
                      provider.loadConfig(_configPathController.text);
                    }
                  },
                  child: const Text('Load'),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                ElevatedButton.icon(
                  onPressed: () => provider.loadConfigDefault(),
                  icon: const Icon(Icons.restore),
                  label: const Text('Load Default'),
                ),
                const SizedBox(width: 16),
                ElevatedButton.icon(
                  onPressed: () => provider.saveConfigDefault(),
                  icon: const Icon(Icons.save),
                  label: const Text('Save as Default'),
                ),
                const SizedBox(width: 16),
                if (_configPathController.text.isNotEmpty)
                  ElevatedButton.icon(
                    onPressed: () => provider.saveConfig(_configPathController.text),
                    icon: const Icon(Icons.save_alt),
                    label: const Text('Save to File'),
                  ),
              ],
            ),
          ],
        ),
      ),
    );
  }

  Widget _buildDeviceSection(CIDeviceProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: ValueListenableBuilder(
          valueListenable: provider.inputDevices,
          builder: (context, inputDevices, _) {
            return ValueListenableBuilder(
              valueListenable: provider.outputDevices,
              builder: (context, outputDevices, _) {
                return ValueListenableBuilder(
                  valueListenable: provider.selectedInputDevice,
                  builder: (context, selectedInput, _) {
                    return ValueListenableBuilder(
                      valueListenable: provider.selectedOutputDevice,
                      builder: (context, selectedOutput, _) {
                        return Column(
                          crossAxisAlignment: CrossAxisAlignment.start,
                          children: [
                            Row(
                              children: [
                                const Icon(Icons.devices, size: 24),
                                const SizedBox(width: 8),
                                Text(
                                  'MIDI Devices',
                                  style: Theme.of(context).textTheme.titleLarge,
                                ),
                                const Spacer(),
                                ElevatedButton.icon(
                                  onPressed: () => provider.refreshDevices(),
                                  icon: const Icon(Icons.refresh),
                                  label: const Text('Refresh'),
                                ),
                              ],
                            ),
                            const SizedBox(height: 16),
                            
                            // Input Devices Section
                            Row(
                              children: [
                                const Icon(Icons.input, size: 20),
                                const SizedBox(width: 8),
                                Text(
                                  'Input Devices (${inputDevices.length})',
                                  style: Theme.of(context).textTheme.titleMedium,
                                ),
                              ],
                            ),
                            const SizedBox(height: 8),
                            Container(
                              width: double.infinity,
                              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                              decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade300),
                                borderRadius: BorderRadius.circular(4),
                              ),
                              child: DropdownButtonHideUnderline(
                                child: DropdownButton<String>(
                                  isExpanded: true,
                                  hint: const Text('Select input device...'),
                                  value: selectedInput,
                                  items: [
                                    const DropdownMenuItem<String>(
                                      value: null,
                                      child: Text('None'),
                                    ),
                                    ...inputDevices.map((device) => DropdownMenuItem<String>(
                                      value: device.deviceId,
                                      child: Text(device.name.isNotEmpty ? device.name : device.deviceId),
                                    )),
                                  ],
                                  onChanged: (String? deviceId) {
                                    if (deviceId != null) {
                                      provider.setInputDevice(deviceId);
                                    }
                                  },
                                ),
                              ),
                            ),
                            
                            const SizedBox(height: 16),
                            
                            // Output Devices Section
                            Row(
                              children: [
                                const Icon(Icons.output, size: 20),
                                const SizedBox(width: 8),
                                Text(
                                  'Output Devices (${outputDevices.length})',
                                  style: Theme.of(context).textTheme.titleMedium,
                                ),
                              ],
                            ),
                            const SizedBox(height: 8),
                            Container(
                              width: double.infinity,
                              padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
                              decoration: BoxDecoration(
                                border: Border.all(color: Colors.grey.shade300),
                                borderRadius: BorderRadius.circular(4),
                              ),
                              child: DropdownButtonHideUnderline(
                                child: DropdownButton<String>(
                                  isExpanded: true,
                                  hint: const Text('Select output device...'),
                                  value: selectedOutput,
                                  items: [
                                    const DropdownMenuItem<String>(
                                      value: null,
                                      child: Text('None'),
                                    ),
                                    ...outputDevices.map((device) => DropdownMenuItem<String>(
                                      value: device.deviceId,
                                      child: Text(device.name.isNotEmpty ? device.name : device.deviceId),
                                    )),
                                  ],
                                  onChanged: (String? deviceId) {
                                    if (deviceId != null) {
                                      provider.setOutputDevice(deviceId);
                                    }
                                  },
                                ),
                              ),
                            ),
                            
                            if (inputDevices.isEmpty && outputDevices.isEmpty) ...[
                              const SizedBox(height: 16),
                              Container(
                                width: double.infinity,
                                padding: const EdgeInsets.all(12),
                                decoration: BoxDecoration(
                                  color: Colors.orange.shade100,
                                  borderRadius: BorderRadius.circular(8),
                                  border: Border.all(color: Colors.orange.shade300),
                                ),
                                child: Row(
                                  children: [
                                    Icon(Icons.warning, color: Colors.orange.shade700),
                                    const SizedBox(width: 8),
                                    Expanded(
                                      child: Text(
                                        'No MIDI devices found. Click "Refresh" to scan for devices.',
                                        style: TextStyle(color: Colors.orange.shade700),
                                      ),
                                    ),
                                  ],
                                ),
                              ),
                            ],
                          ],
                        );
                      },
                    );
                  },
                );
              },
            );
          },
        ),
      ),
    );
  }

  Widget _buildApplicationSection(CIToolProvider provider) {
    return Card(
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                const Icon(Icons.info, size: 24),
                const SizedBox(width: 8),
                Text(
                  'Application',
                  style: Theme.of(context).textTheme.titleLarge,
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                const Text('Status: '),
                Container(
                  padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                  decoration: BoxDecoration(
                    color: provider.isInitialized ? Colors.green.shade100 : Colors.red.shade100,
                    borderRadius: BorderRadius.circular(12),
                    border: Border.all(
                      color: provider.isInitialized ? Colors.green.shade300 : Colors.red.shade300,
                    ),
                  ),
                  child: Row(
                    mainAxisSize: MainAxisSize.min,
                    children: [
                      Icon(
                        provider.isInitialized ? Icons.check_circle : Icons.error,
                        size: 16,
                        color: provider.isInitialized ? Colors.green.shade700 : Colors.red.shade700,
                      ),
                      const SizedBox(width: 4),
                      Text(
                        provider.isInitialized ? 'Initialized' : 'Not Initialized',
                        style: TextStyle(
                          color: provider.isInitialized ? Colors.green.shade700 : Colors.red.shade700,
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                    ],
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            const Text('Flutter MIDI-CI Tool v1.0.0'),
            const SizedBox(height: 8),
            const Text('Built with midicci C++ library'),
            const SizedBox(height: 16),
            Row(
              children: [
                if (!provider.isInitialized)
                  ElevatedButton.icon(
                    onPressed: () => provider.initialize(),
                    icon: const Icon(Icons.play_arrow),
                    label: const Text('Initialize'),
                  ),
                if (provider.isInitialized) ...[
                  ElevatedButton.icon(
                    onPressed: () => provider.shutdown(),
                    icon: const Icon(Icons.stop),
                    label: const Text('Shutdown'),
                  ),
                  const SizedBox(width: 16),
                  ElevatedButton.icon(
                    onPressed: () => provider.initialize(),
                    icon: const Icon(Icons.refresh),
                    label: const Text('Restart'),
                  ),
                ],
              ],
            ),
          ],
        ),
      ),
    );
  }
}
