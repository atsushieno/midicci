import 'package:flutter/material.dart';
import '../models/ci_device_model.dart';

class PropertyEditor extends StatelessWidget {
  final List<Property> properties;
  final Function(Property) onAdd;
  final Function(Property) onRemove;
  final Function(String, String, List<int>) onUpdate;

  const PropertyEditor({
    super.key,
    required this.properties,
    required this.onAdd,
    required this.onRemove,
    required this.onUpdate,
  });

  @override
  Widget build(BuildContext context) {
    if (properties.isEmpty) {
      return const Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            Icon(
              Icons.settings,
              size: 64,
              color: Colors.grey,
            ),
            SizedBox(height: 16),
            Text(
              'No properties configured',
              style: TextStyle(
                fontSize: 18,
                color: Colors.grey,
              ),
            ),
            SizedBox(height: 8),
            Text(
              'Add properties to expose device configuration',
              style: TextStyle(color: Colors.grey),
            ),
          ],
        ),
      );
    }

    return ListView.builder(
      itemCount: properties.length,
      itemBuilder: (context, index) {
        final property = properties[index];
        return PropertyCard(
          property: property,
          onRemove: () => onRemove(property),
          onUpdate: (data) => onUpdate(property.propertyId, property.resourceId, data),
        );
      },
    );
  }
}

class PropertyCard extends StatefulWidget {
  final Property property;
  final VoidCallback onRemove;
  final Function(List<int>) onUpdate;

  const PropertyCard({
    super.key,
    required this.property,
    required this.onRemove,
    required this.onUpdate,
  });

  @override
  State<PropertyCard> createState() => _PropertyCardState();
}

class _PropertyCardState extends State<PropertyCard> {
  late TextEditingController _dataController;
  bool _isEditing = false;

  @override
  void initState() {
    super.initState();
    _dataController = TextEditingController(
      text: String.fromCharCodes(widget.property.data),
    );
  }

  @override
  void dispose() {
    _dataController.dispose();
    super.dispose();
  }

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
                const Icon(Icons.settings, color: Colors.blue),
                const SizedBox(width: 8),
                Expanded(
                  child: Column(
                    crossAxisAlignment: CrossAxisAlignment.start,
                    children: [
                      Text(
                        widget.property.name.isNotEmpty 
                            ? widget.property.name 
                            : widget.property.propertyId,
                        style: Theme.of(context).textTheme.titleMedium?.copyWith(
                          fontWeight: FontWeight.bold,
                        ),
                      ),
                      Text(
                        '${widget.property.propertyId} / ${widget.property.resourceId}',
                        style: Theme.of(context).textTheme.bodySmall?.copyWith(
                          color: Colors.grey.shade600,
                        ),
                      ),
                    ],
                  ),
                ),
                IconButton(
                  onPressed: () {
                    setState(() {
                      _isEditing = !_isEditing;
                    });
                  },
                  icon: Icon(_isEditing ? Icons.save : Icons.edit),
                  color: _isEditing ? Colors.green : Colors.blue,
                ),
                IconButton(
                  onPressed: widget.onRemove,
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
                  widget.property.mediaType,
                  Icons.description,
                ),
                const SizedBox(width: 8),
                _buildInfoChip(
                  context,
                  '${widget.property.data.length} bytes',
                  Icons.data_usage,
                ),
              ],
            ),
            const SizedBox(height: 12),
            if (_isEditing) ...[
              TextField(
                controller: _dataController,
                decoration: const InputDecoration(
                  labelText: 'Property Value',
                  border: OutlineInputBorder(),
                ),
                maxLines: 3,
                onSubmitted: (value) {
                  widget.onUpdate(value.codeUnits);
                  setState(() {
                    _isEditing = false;
                  });
                },
              ),
              const SizedBox(height: 8),
              Row(
                children: [
                  ElevatedButton(
                    onPressed: () {
                      widget.onUpdate(_dataController.text.codeUnits);
                      setState(() {
                        _isEditing = false;
                      });
                    },
                    child: const Text('Save'),
                  ),
                  const SizedBox(width: 8),
                  TextButton(
                    onPressed: () {
                      _dataController.text = String.fromCharCodes(widget.property.data);
                      setState(() {
                        _isEditing = false;
                      });
                    },
                    child: const Text('Cancel'),
                  ),
                ],
              ),
            ] else ...[
              Container(
                width: double.infinity,
                padding: const EdgeInsets.all(12),
                decoration: BoxDecoration(
                  color: Colors.grey.shade100,
                  borderRadius: BorderRadius.circular(8),
                  border: Border.all(color: Colors.grey.shade300),
                ),
                child: Text(
                  widget.property.data.isEmpty 
                      ? '(empty)'
                      : String.fromCharCodes(widget.property.data),
                  style: const TextStyle(fontFamily: 'monospace'),
                ),
              ),
            ],
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
