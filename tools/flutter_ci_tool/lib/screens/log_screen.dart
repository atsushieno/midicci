import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import '../providers/ci_tool_provider.dart';

class LogScreen extends StatefulWidget {
  const LogScreen({super.key});

  @override
  State<LogScreen> createState() => _LogScreenState();
}

class _LogScreenState extends State<LogScreen> {
  final ScrollController _scrollController = ScrollController();
  bool _autoScroll = true;

  @override
  void dispose() {
    _scrollController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Consumer<CIToolProvider>(
      builder: (context, provider, child) {
        return Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              // Debug information card
              if (provider.lastLogRefreshStatus != null || provider.logRefreshCallCount > 0)
                Card(
                  color: Colors.blue.shade50,
                  child: Padding(
                    padding: const EdgeInsets.all(12.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Icon(Icons.bug_report, size: 20, color: Colors.blue.shade700),
                            const SizedBox(width: 8),
                            Text(
                              'Logging Debug Info',
                              style: TextStyle(
                                fontWeight: FontWeight.bold,
                                color: Colors.blue.shade700,
                              ),
                            ),
                            const Spacer(),
                            Text(
                              'Refresh calls: ${provider.logRefreshCallCount}',
                              style: TextStyle(
                                fontSize: 12,
                                color: Colors.blue.shade600,
                              ),
                            ),
                          ],
                        ),
                        const SizedBox(height: 8),
                        if (provider.lastLogRefreshStatus != null)
                          Text(
                            'Last refresh: ${provider.lastLogRefreshStatus}',
                            style: TextStyle(
                              fontSize: 12,
                              fontFamily: 'monospace',
                              color: Colors.blue.shade800,
                            ),
                          ),
                        if (provider.lastNativeLogsJson != null)
                          Text(
                            'Native JSON: ${provider.lastNativeLogsJson!.length > 100 ? "${provider.lastNativeLogsJson!.substring(0, 100)}..." : provider.lastNativeLogsJson}',
                            style: TextStyle(
                              fontSize: 10,
                              fontFamily: 'monospace',
                              color: Colors.blue.shade600,
                            ),
                          ),
                      ],
                    ),
                  ),
                ),
              const SizedBox(height: 8),
              
              Card(
                child: Padding(
                  padding: const EdgeInsets.all(16.0),
                  child: Row(
                    children: [
                      const Icon(Icons.list_alt, size: 24),
                      const SizedBox(width: 8),
                      Text(
                        'MIDI-CI Message Log',
                        style: Theme.of(context).textTheme.titleLarge,
                      ),
                      const Spacer(),
                      Wrap(
                        spacing: 8,
                        children: [
                          Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              Switch(
                                value: _autoScroll,
                                onChanged: (value) {
                                  setState(() {
                                    _autoScroll = value;
                                  });
                                },
                              ),
                              const SizedBox(width: 8),
                              const Text('Auto-scroll'),
                            ],
                          ),
                          ElevatedButton.icon(
                            onPressed: () => provider.refreshLogs(),
                            icon: const Icon(Icons.refresh),
                            label: const Text('Refresh'),
                          ),
                          ElevatedButton.icon(
                            onPressed: () => provider.clearLogs(),
                            icon: const Icon(Icons.clear),
                            label: const Text('Clear'),
                          ),
                        ],
                      ),
                    ],
                  ),
                ),
              ),
              const SizedBox(height: 16),
              Expanded(
                child: Card(
                  child: Container(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.start,
                      children: [
                        Row(
                          children: [
                            Text(
                              'Messages (${provider.logs.length})',
                              style: Theme.of(context).textTheme.titleMedium,
                            ),
                            const Spacer(),
                            if (provider.logs.isNotEmpty)
                              Text(
                                'Latest: ${_formatTimestamp(DateTime.now())}',
                                style: Theme.of(context).textTheme.bodySmall,
                              ),
                          ],
                        ),
                        const SizedBox(height: 16),
                        Expanded(
                          child: provider.logs.isEmpty
                              ? const Center(
                                  child: Column(
                                    mainAxisAlignment: MainAxisAlignment.center,
                                    children: [
                                      Icon(
                                        Icons.message,
                                        size: 64,
                                        color: Colors.grey,
                                      ),
                                      SizedBox(height: 16),
                                      Text(
                                        'No messages logged yet',
                                        style: TextStyle(
                                          fontSize: 18,
                                          color: Colors.grey,
                                        ),
                                      ),
                                      SizedBox(height: 8),
                                      Text(
                                        'MIDI-CI messages will appear here',
                                        style: TextStyle(color: Colors.grey),
                                      ),
                                    ],
                                  ),
                                )
                              : ListView.builder(
                                  controller: _scrollController,
                                  itemCount: provider.logs.length,
                                  itemBuilder: (context, index) {
                                    final log = provider.logs[index];
                                    return _buildLogEntry(context, log, index);
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

  Widget _buildLogEntry(BuildContext context, String logEntry, int index) {
    final isOutgoing = logEntry.contains('OUT:') || logEntry.contains('->');
    final isIncoming = logEntry.contains('IN:') || logEntry.contains('<-');
    final isError = logEntry.toLowerCase().contains('error');
    
    Color? backgroundColor;
    Color? textColor;
    IconData icon = Icons.message;
    
    if (isError) {
      backgroundColor = Colors.red.shade50;
      textColor = Colors.red.shade800;
      icon = Icons.error;
    } else if (isOutgoing) {
      backgroundColor = Colors.blue.shade50;
      textColor = Colors.blue.shade800;
      icon = Icons.arrow_forward;
    } else if (isIncoming) {
      backgroundColor = Colors.green.shade50;
      textColor = Colors.green.shade800;
      icon = Icons.arrow_back;
    }

    return Container(
      margin: const EdgeInsets.only(bottom: 4),
      padding: const EdgeInsets.all(12),
      decoration: BoxDecoration(
        color: backgroundColor,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(
          color: backgroundColor?.withOpacity(0.3) ?? Colors.grey.shade300,
        ),
      ),
      child: Row(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          Icon(
            icon,
            size: 16,
            color: textColor ?? Theme.of(context).textTheme.bodyMedium?.color,
          ),
          const SizedBox(width: 8),
          Text(
            (index + 1).toString().padLeft(3, '0'),
            style: TextStyle(
              fontFamily: 'monospace',
              fontSize: 12,
              color: Colors.grey.shade600,
            ),
          ),
          const SizedBox(width: 8),
          Expanded(
            child: Text(
              logEntry,
              style: TextStyle(
                fontFamily: 'monospace',
                fontSize: 13,
                color: textColor ?? Theme.of(context).textTheme.bodyMedium?.color,
              ),
            ),
          ),
        ],
      ),
    );
  }

  String _formatTimestamp(DateTime timestamp) {
    return '${timestamp.hour.toString().padLeft(2, '0')}:'
           '${timestamp.minute.toString().padLeft(2, '0')}:'
           '${timestamp.second.toString().padLeft(2, '0')}';
  }
}
