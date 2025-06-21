import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_device_provider.dart';
import '../models/ci_device_model.dart';

class ResponderScreen extends StatefulWidget {
  const ResponderScreen({super.key});

  @override
  State<ResponderScreen> createState() => _ResponderScreenState();
}

class _ResponderScreenState extends State<ResponderScreen> {
  @override
  void initState() {
    super.initState();
    // Setup reactive listeners for local profiles and properties
    WidgetsBinding.instance.addPostFrameCallback((_) {
      final deviceProvider = Provider.of<CIDeviceProvider>(context, listen: false);
      deviceProvider.addProfilesUpdatedCallback(() {
        if (mounted) setState(() {});
      });
      deviceProvider.addPropertiesUpdatedCallback(() {
        if (mounted) setState(() {});
      });
    });
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CIDeviceProvider>(
      builder: (context, deviceProvider, child) {
        return Padding(
          padding: const EdgeInsets.all(16.0),
          child: SingleChildScrollView(
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.start,
              children: [
                // Header Card
                Card(
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            const Icon(Icons.settings_input_antenna, size: 32),
                            const SizedBox(width: 16),
                            Column(
                              crossAxisAlignment: CrossAxisAlignment.start,
                              children: [
                                Text(
                                  'Responder Mode',
                                  style: Theme.of(context).textTheme.titleLarge,
                                ),
                                const Text(
                                  'Manage local profiles and properties',
                                  style: TextStyle(color: Colors.grey),
                                ),
                              ],
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                ),
                
                const SizedBox(height: 16),
                
                // Local Profiles Card
                SizedBox(
                  height: 300,
                  child: Card(
                    child: Padding(
                      padding: const EdgeInsets.all(16.0),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Text(
                                'Local Profiles',
                                style: Theme.of(context).textTheme.titleMedium,
                              ),
                              ElevatedButton.icon(
                                onPressed: () => _showAddProfileDialog(context, deviceProvider),
                                icon: const Icon(Icons.add),
                                label: const Text('Add Profile'),
                              ),
                            ],
                          ),
                          const SizedBox(height: 16),
                          Expanded(
                            // Reactive local profiles list
                            child: ValueListenableBuilder(
                              valueListenable: deviceProvider.localProfiles,
                              builder: (context, profiles, child) {
                                return profiles.isEmpty
                                    ? const Center(
                                        child: Text('No local profiles configured'),
                                      )
                                    : ListView.builder(
                                        itemCount: profiles.length,
                                        itemBuilder: (context, index) {
                                          final profile = profiles[index];
                                          return ListTile(
                                            leading: Icon(
                                              profile.enabled ? Icons.check_circle : Icons.circle_outlined,
                                              color: profile.enabled ? Colors.green : Colors.grey,
                                            ),
                                            title: Text(profile.profileId),
                                            subtitle: Text('Group: ${profile.group}, Address: ${profile.address}, Channels: ${profile.numChannelsRequested}'),
                                            trailing: Row(
                                              mainAxisSize: MainAxisSize.min,
                                              children: [
                                                Switch(
                                                  value: profile.enabled,
                                                  onChanged: (enabled) {
                                                    // TODO: Update profile enabled state
                                                  },
                                                ),
                                                IconButton(
                                                  icon: const Icon(Icons.delete),
                                                  onPressed: () {
                                                    deviceProvider.removeLocalProfile(
                                                      profile.group,
                                                      profile.address,
                                                      profile.profileId,
                                                    );
                                                  },
                                                ),
                                              ],
                                            ),
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
                
                const SizedBox(height: 16),
                
                // Local Properties Card
                SizedBox(
                  height: 300,
                  child: Card(
                    child: Padding(
                      padding: const EdgeInsets.all(16.0),
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Row(
                            mainAxisAlignment: MainAxisAlignment.spaceBetween,
                            children: [
                              Text(
                                'Local Properties',
                                style: Theme.of(context).textTheme.titleMedium,
                              ),
                              ElevatedButton.icon(
                                onPressed: () => _showAddPropertyDialog(context, deviceProvider),
                                icon: const Icon(Icons.add),
                                label: const Text('Add Property'),
                              ),
                            ],
                          ),
                          const SizedBox(height: 16),
                          Expanded(
                            // Reactive local properties list
                            child: ValueListenableBuilder(
                              valueListenable: deviceProvider.localProperties,
                              builder: (context, properties, child) {
                                return properties.isEmpty
                                    ? const Center(
                                        child: Text('No local properties configured'),
                                      )
                                    : ListView.builder(
                                        itemCount: properties.length,
                                        itemBuilder: (context, index) {
                                          final property = properties[index];
                                          return ListTile(
                                            leading: const Icon(Icons.settings),
                                            title: Text(property.name),
                                            subtitle: Column(
                                              crossAxisAlignment: CrossAxisAlignment.start,
                                              children: [
                                                Text('ID: ${property.propertyId}'),
                                                Text('Type: ${property.mediaType}'),
                                                Text('Size: ${property.data.length} bytes'),
                                              ],
                                            ),
                                            trailing: Row(
                                              mainAxisSize: MainAxisSize.min,
                                              children: [
                                                IconButton(
                                                  icon: const Icon(Icons.edit),
                                                  onPressed: () {
                                                    // TODO: Edit property
                                                  },
                                                ),
                                                IconButton(
                                                  icon: const Icon(Icons.delete),
                                                  onPressed: () {
                                                    deviceProvider.removeLocalProperty(property.propertyId);
                                                  },
                                                ),
                                              ],
                                            ),
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

  void _showAddProfileDialog(BuildContext context, CIDeviceProvider deviceProvider) {
    final profileIdController = TextEditingController();
    int group = 0;
    int address = 0;
    int numChannels = 1;
    bool enabled = true;

    showDialog(
      context: context,
      builder: (context) => StatefulBuilder(
        builder: (context, setDialogState) => AlertDialog(
          title: const Text('Add Local Profile'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            children: [
              TextField(
                controller: profileIdController,
                decoration: const InputDecoration(
                  labelText: 'Profile ID',
                  hintText: 'e.g., 0x7E,0x00,0x01',
                ),
              ),
              const SizedBox(height: 16),
              Row(
                children: [
                  Expanded(
                    child: TextField(
                      decoration: const InputDecoration(labelText: 'Group'),
                      keyboardType: TextInputType.number,
                      onChanged: (value) => group = int.tryParse(value) ?? 0,
                    ),
                  ),
                  const SizedBox(width: 16),
                  Expanded(
                    child: TextField(
                      decoration: const InputDecoration(labelText: 'Address'),
                      keyboardType: TextInputType.number,
                      onChanged: (value) => address = int.tryParse(value) ?? 0,
                    ),
                  ),
                ],
              ),
              const SizedBox(height: 16),
              TextField(
                decoration: const InputDecoration(labelText: 'Channels'),
                keyboardType: TextInputType.number,
                onChanged: (value) => numChannels = int.tryParse(value) ?? 1,
              ),
              const SizedBox(height: 16),
              CheckboxListTile(
                title: const Text('Enabled'),
                value: enabled,
                onChanged: (value) => setDialogState(() => enabled = value ?? true),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Cancel'),
            ),
            ElevatedButton(
              onPressed: () {
                final profile = Profile(
                  profileId: profileIdController.text,
                  group: group,
                  address: address,
                  enabled: enabled,
                  numChannelsRequested: numChannels,
                );
                deviceProvider.addLocalProfile(profile);
                Navigator.pop(context);
              },
              child: const Text('Add'),
            ),
          ],
        ),
      ),
    );
  }

  void _showAddPropertyDialog(BuildContext context, CIDeviceProvider deviceProvider) {
    final propertyIdController = TextEditingController();
    final nameController = TextEditingController();
    final mediaTypeController = TextEditingController(text: 'application/json');
    final dataController = TextEditingController();

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
                hintText: 'e.g., DeviceInfo',
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
              decoration: const InputDecoration(
                labelText: 'Media Type',
                hintText: 'e.g., application/json',
              ),
            ),
            const SizedBox(height: 16),
            TextField(
              controller: dataController,
              decoration: const InputDecoration(
                labelText: 'Initial Data (JSON)',
                hintText: '{"version": "1.0"}',
              ),
              maxLines: 3,
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(context),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            onPressed: () {
              // Convert JSON string to bytes
              final dataBytes = dataController.text.codeUnits;
              
              final property = Property(
                propertyId: propertyIdController.text,
                resourceId: propertyIdController.text, // Use same as property ID
                name: nameController.text,
                mediaType: mediaTypeController.text,
                data: dataBytes,
              );
              deviceProvider.addLocalProperty(property);
              Navigator.pop(context);
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  @override
  void dispose() {
    super.dispose();
  }
}