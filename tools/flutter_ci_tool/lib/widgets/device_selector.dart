import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_device_provider.dart';
import '../models/ci_device_model.dart';

class DeviceSelector extends StatelessWidget {
  const DeviceSelector({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<CIDeviceProvider>(
      builder: (context, provider, child) {
        return Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Expanded(
                  child: _buildDeviceDropdown(
                    context,
                    'Input Device',
                    provider.inputDevices.items,
                    provider.selectedInputDevice.get(),
                    (deviceId) => provider.setInputDevice(deviceId),
                    Icons.input,
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: _buildDeviceDropdown(
                    context,
                    'Output Device',
                    provider.outputDevices.items,
                    provider.selectedOutputDevice.get(),
                    (deviceId) => provider.setOutputDevice(deviceId),
                    Icons.output,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Text(
                  'Available devices: ${provider.inputDevices.length} input, ${provider.outputDevices.length} output',
                  style: Theme.of(context).textTheme.bodySmall,
                ),
                const Spacer(),
                TextButton.icon(
                  onPressed: () => provider.refreshDevices(),
                  icon: const Icon(Icons.refresh, size: 16),
                  label: const Text('Refresh'),
                ),
              ],
            ),
          ],
        );
      },
    );
  }

  Widget _buildDeviceDropdown(
    BuildContext context,
    String label,
    List<MidiDevice> devices,
    String? selectedDeviceId,
    Function(String) onChanged,
    IconData icon,
  ) {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Row(
          children: [
            Icon(icon, size: 16),
            const SizedBox(width: 4),
            Text(
              label,
              style: Theme.of(context).textTheme.titleSmall,
            ),
          ],
        ),
        const SizedBox(height: 8),
        Container(
          width: double.infinity,
          padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 4),
          decoration: BoxDecoration(
            border: Border.all(color: Colors.grey.shade300),
            borderRadius: BorderRadius.circular(8),
          ),
          child: DropdownButtonHideUnderline(
            child: DropdownButton<String>(
              value: selectedDeviceId,
              hint: Text('Select $label'),
              isExpanded: true,
              items: [
                const DropdownMenuItem<String>(
                  value: null,
                  child: Text('None'),
                ),
                ...devices.map((device) => DropdownMenuItem<String>(
                  value: device.deviceId,
                  child: Text(
                    device.name,
                    overflow: TextOverflow.ellipsis,
                  ),
                )),
              ],
              onChanged: (value) {
                if (value != null) {
                  onChanged(value);
                }
              },
            ),
          ),
        ),
      ],
    );
  }
}
