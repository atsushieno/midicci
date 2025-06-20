import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_device_provider.dart';
import '../models/ci_device_model.dart';
import '../widgets/profile_list.dart';
import '../widgets/property_editor.dart';

class ResponderScreen extends StatefulWidget {
  const ResponderScreen({super.key});

  @override
  State<ResponderScreen> createState() => _ResponderScreenState();
}

class _ResponderScreenState extends State<ResponderScreen> with TickerProviderStateMixin {
  late TabController _tabController;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 2, vsync: this);
  }

  @override
  void dispose() {
    _tabController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CIDeviceProvider>(
      builder: (context, provider, child) {
        return Column(
          children: [
            Container(
              color: Theme.of(context).colorScheme.surface,
              child: TabBar(
                controller: _tabController,
                tabs: const [
                  Tab(icon: Icon(Icons.account_tree), text: 'Profiles'),
                  Tab(icon: Icon(Icons.settings), text: 'Properties'),
                ],
              ),
            ),
            Expanded(
              child: TabBarView(
                controller: _tabController,
                children: [
                  _buildProfilesTab(provider),
                  _buildPropertiesTab(provider),
                ],
              ),
            ),
          ],
        );
      },
    );
  }

  Widget _buildProfilesTab(CIDeviceProvider provider) {
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
                      const Icon(Icons.account_tree, size: 24),
                      const SizedBox(width: 8),
                      Text(
                        'Local Profiles',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const Spacer(),
                      ElevatedButton.icon(
                        onPressed: () => _showAddProfileDialog(context, provider),
                        icon: const Icon(Icons.add),
                        label: const Text('Add Profile'),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Configure local MIDI-CI profiles that this device supports. '
                    'These profiles will be advertised to remote devices.',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),
          Expanded(
            child: ProfileList(
              profiles: provider.localProfiles,
              onRemove: (profile) => provider.removeLocalProfile(profile),
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
  }

  Widget _buildPropertiesTab(CIDeviceProvider provider) {
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
                      const Icon(Icons.settings, size: 24),
                      const SizedBox(width: 8),
                      Text(
                        'Local Properties',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const Spacer(),
                      ElevatedButton.icon(
                        onPressed: () => _showAddPropertyDialog(context, provider),
                        icon: const Icon(Icons.add),
                        label: const Text('Add Property'),
                      ),
                    ],
                  ),
                  const SizedBox(height: 8),
                  Text(
                    'Configure local MIDI-CI properties that this device exposes. '
                    'Remote devices can query and modify these properties.',
                    style: Theme.of(context).textTheme.bodyMedium,
                  ),
                ],
              ),
            ),
          ),
          const SizedBox(height: 16),
          Expanded(
            child: PropertyEditor(
              properties: provider.localProperties,
              onAdd: (property) => provider.addLocalProperty(property),
              onRemove: (property) => provider.removeLocalProperty(property),
              onUpdate: (propertyId, resourceId, data) => 
                  provider.updatePropertyValue(propertyId, resourceId, data),
            ),
          ),
        ],
      ),
    );
  }

  void _showAddProfileDialog(BuildContext context, CIDeviceProvider provider) {
    final profileIdController = TextEditingController();
    final groupController = TextEditingController(text: '0');
    final addressController = TextEditingController(text: '0');
    final channelsController = TextEditingController(text: '1');

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Add Local Profile'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: profileIdController,
              decoration: const InputDecoration(
                labelText: 'Profile ID',
                hintText: 'e.g., 0x7E7F',
              ),
            ),
            const SizedBox(height: 16),
            Row(
              children: [
                Expanded(
                  child: TextField(
                    controller: groupController,
                    decoration: const InputDecoration(labelText: 'Group'),
                    keyboardType: TextInputType.number,
                  ),
                ),
                const SizedBox(width: 16),
                Expanded(
                  child: TextField(
                    controller: addressController,
                    decoration: const InputDecoration(labelText: 'Address'),
                    keyboardType: TextInputType.number,
                  ),
                ),
              ],
            ),
            const SizedBox(height: 16),
            TextField(
              controller: channelsController,
              decoration: const InputDecoration(labelText: 'Channels'),
              keyboardType: TextInputType.number,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              final profile = Profile(
                profileId: profileIdController.text,
                group: int.tryParse(groupController.text) ?? 0,
                address: int.tryParse(addressController.text) ?? 0,
                enabled: true,
                numChannelsRequested: int.tryParse(channelsController.text) ?? 1,
              );
              provider.addLocalProfile(profile);
              Navigator.of(context).pop();
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  void _showAddPropertyDialog(BuildContext context, CIDeviceProvider provider) {
    final propertyIdController = TextEditingController();
    final resourceIdController = TextEditingController();
    final nameController = TextEditingController();
    final mediaTypeController = TextEditingController(text: 'application/json');

    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Add Local Property'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: propertyIdController,
              decoration: const InputDecoration(
                labelText: 'Property ID',
                hintText: 'e.g., device-info',
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: resourceIdController,
              decoration: const InputDecoration(
                labelText: 'Resource ID',
                hintText: 'e.g., manufacturer',
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: nameController,
              decoration: const InputDecoration(
                labelText: 'Display Name',
                hintText: 'e.g., Device Information',
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: mediaTypeController,
              decoration: const InputDecoration(labelText: 'Media Type'),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.of(context).pop(),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              final property = Property(
                propertyId: propertyIdController.text,
                resourceId: resourceIdController.text,
                name: nameController.text,
                mediaType: mediaTypeController.text,
                data: [],
              );
              provider.addLocalProperty(property);
              Navigator.of(context).pop();
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }
}
