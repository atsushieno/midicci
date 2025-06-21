import 'package:flutter/material.dart';
import '../models/ci_device_model.dart';

class ProfileList extends StatelessWidget {
  final List<Profile> profiles;
  final Function(Profile) onRemove;

  const ProfileList({
    super.key,
    required this.profiles,
    required this.onRemove,
  });

  @override
  Widget build(BuildContext context) {
    if (profiles.isEmpty) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.account_tree,
              size: 64,
              color: Colors.grey,
            ),
            SizedBox(height: 16),
            Text(
              'No profiles configured',
              style: TextStyle(
                fontSize: 18,
                color: Colors.grey,
              ),
            ),
            SizedBox(height: 8),
            Text(
              'Add profiles to advertise MIDI-CI capabilities',
              style: TextStyle(color: Colors.grey),
            ),
          ],
        ),
      );
    }

    return ListView.builder(
      itemCount: profiles.length,
      itemBuilder: (context, index) {
        final profile = profiles[index];
        return ProfileCard(
          profile: profile,
          onRemove: () => onRemove(profile),
        );
      },
    );
  }
}

class ProfileCard extends StatelessWidget {
  final Profile profile;
  final VoidCallback onRemove;

  const ProfileCard({
    super.key,
    required this.profile,
    required this.onRemove,
  });

  @override
  Widget build(BuildContext context) {
    return Card(
      margin: const EdgeInsets.only(bottom: 8),
      child: Padding(
        padding: const EdgeInsets.all(16.0),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Row(
              children: [
                Icon(
                  profile.enabled ? Icons.check_circle : Icons.radio_button_unchecked,
                  color: profile.enabled ? Colors.green : Colors.grey,
                ),
                const SizedBox(width: 8),
                Text(
                  profile.profileId,
                  style: Theme.of(context).textTheme.titleMedium?.copyWith(
                    fontWeight: FontWeight.bold,
                  ),
                ),
                const Spacer(),
                IconButton(
                  onPressed: onRemove,
                  icon: const Icon(Icons.delete),
                  color: Colors.red,
                ),
              ],
            ),
            const SizedBox(height: 8),
            Row(
              children: [
                _buildInfoChip(
                  context,
                  'Group: ${profile.group}',
                  Icons.group,
                ),
                const SizedBox(width: 8),
                _buildInfoChip(
                  context,
                  'Address: ${profile.address}',
                  Icons.location_on,
                ),
                const SizedBox(width: 8),
                _buildInfoChip(
                  context,
                  'Channels: ${profile.numChannelsRequested}',
                  Icons.queue_music,
                ),
              ],
            ),
          ],
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
          Icon(icon, size: 14),
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
