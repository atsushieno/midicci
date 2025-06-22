import 'dart:async';
import 'dart:convert';
import 'package:flutter/foundation.dart';
import '../native/midi_ci_bridge.dart';
import '../models/ci_device_model.dart';

class CIToolProvider extends ChangeNotifier {
  bool _isInitialized = false;
  String _configPath = '';
  final List<String> _logs = [];
  String? _lastError;
  bool _libraryLoaded = false;
  String? _libraryLoadError;
  int _lastNativeLogCount = 0;
  
  // Debug information for logging system
  String? _lastLogRefreshStatus;
  String? _lastNativeLogsJson;
  int _logRefreshCallCount = 0;
  
  bool get isInitialized => _isInitialized;
  String get configPath => _configPath;
  List<String> get logs => List.unmodifiable(_logs);
  String? get lastError => _lastError;
  bool get libraryLoaded => _libraryLoaded;
  String? get libraryLoadError => _libraryLoadError;
  
  // Debug getters for logging system
  String? get lastLogRefreshStatus => _lastLogRefreshStatus;
  String? get lastNativeLogsJson => _lastNativeLogsJson;
  int get logRefreshCallCount => _logRefreshCallCount;
  
  Future<bool> initialize() async {
    try {
      _lastError = null;
      _libraryLoadError = null;
      
      // First, try to test if the library can be loaded
      _addLog('Testing native library loading...');
      try {
        // Test library loading by trying to access the bridge
        MidiCIBridge.instance.isInitialized;
        _libraryLoaded = true;
        _addLog('✅ Native library loaded successfully!');
      } catch (e) {
        _libraryLoaded = false;
        _libraryLoadError = e.toString();
        _addLog('❌ Failed to load native library: $e');
        _lastError = 'Library loading failed: $e';
        notifyListeners();
        return false;
      }
      
      _addLog('Initializing MIDI-CI repository...');
      final bridgeInitialized = await MidiCIBridge.instance.initialize();
      if (!bridgeInitialized) {
        _lastError = 'Failed to initialize native bridge';
        _addLog('❌ Bridge initialization failed');
        notifyListeners();
        return false;
      }
      
      _isInitialized = true;
      _addLog('✅ MIDI-CI Tool initialized successfully');
      
      // Note: Using simple log refresh after operations instead of real-time callbacks due to thread safety
      
      // Initial refresh of any existing logs
      await _refreshNativeLogs();
      
      // DEBUG: Automatically test logging by adding a test log
      MidiCIBridge.instance.log("DEBUG: Test log from Flutter initialization", isOutgoing: true);
      await Future.delayed(const Duration(milliseconds: 200));
      await _refreshNativeLogs();
      
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Initialization error: $e';
      _libraryLoadError = e.toString();
      _isInitialized = false;
      _addLog('❌ Initialization error: $e');
      notifyListeners();
      return false;
    }
  }
  
  Future<void> shutdown() async {
    try {
      if (_isInitialized) {
        MidiCIBridge.instance.shutdown();
        _addLog('MIDI-CI Tool shutdown');
      }
      _isInitialized = false;
      _lastError = null;
      notifyListeners();
    } catch (e) {
      _lastError = 'Shutdown error: $e';
      notifyListeners();
    }
  }
  
  Future<bool> loadConfig(String configPath) async {
    try {
      _lastError = null;
      // TODO: Implement config loading via FFI
      _configPath = configPath;
      _addLog('Configuration loaded from: $configPath');
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Load config error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> saveConfig(String configPath) async {
    try {
      _lastError = null;
      // TODO: Implement config saving via FFI
      _configPath = configPath;
      _addLog('Configuration saved to: $configPath');
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Save config error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<void> loadConfigDefault() async {
    try {
      _lastError = null;
      // TODO: Implement default config loading via FFI
      _configPath = 'default';
      _addLog('Default configuration loaded');
      notifyListeners();
    } catch (e) {
      _lastError = 'Load default config error: $e';
      notifyListeners();
    }
  }
  
  Future<void> saveConfigDefault() async {
    try {
      _lastError = null;
      // TODO: Implement default config saving via FFI
      _addLog('Default configuration saved');
      notifyListeners();
    } catch (e) {
      _lastError = 'Save default config error: $e';
      notifyListeners();
    }
  }
  
  void clearLogs() {
    _logs.clear();
    _lastNativeLogCount = 0;
    notifyListeners();
  }
  
  void _addLog(String message) {
    final timestamp = DateTime.now().toIso8601String();
    final logEntry = '[$timestamp] $message';
    _logs.add(logEntry);
    
    // Also log to native side
    if (_isInitialized) {
      MidiCIBridge.instance.log(message);
    }
    
    notifyListeners();
  }
  
  int getMuid() {
    if (_isInitialized) {
      return MidiCIBridge.instance.getMuid();
    }
    return 0;
  }
  
  /// Manually refresh logs from native side (call after MIDI-CI operations)
  Future<void> refreshLogs() async {
    _logRefreshCallCount++;
    _lastLogRefreshStatus = 'refreshLogs() called #$_logRefreshCallCount';
    
    if (kDebugMode) {
      debugPrint('🔄 Manual log refresh called (#$_logRefreshCallCount)');
    }
    
    await _refreshNativeLogs();
  }


  /// Refresh logs from native side and merge with local logs
  Future<void> _refreshNativeLogs() async {
    if (!_isInitialized) {
      _lastLogRefreshStatus = 'Skipped: not initialized';
      if (kDebugMode) {
        debugPrint('⚠️ Refresh logs skipped - not initialized');
      }
      return;
    }
    
    try {
      if (kDebugMode) {
        debugPrint('🔍 Calling native getLogsJson()...');
      }
      final nativeLogsJson = MidiCIBridge.instance.getLogsJson();
      _lastNativeLogsJson = nativeLogsJson;
      
      if (kDebugMode) {
        debugPrint('📥 Native logs JSON length: ${nativeLogsJson.length}');
        debugPrint('📥 Native logs JSON content: $nativeLogsJson');
        debugPrint('📊 Current Flutter logs: ${_logs.length}, Last native count: $_lastNativeLogCount');
      }
      
      if (nativeLogsJson.isNotEmpty && nativeLogsJson != '[]') {
        final List<dynamic> jsonList = json.decode(nativeLogsJson);
        
        if (kDebugMode) {
          debugPrint('📋 Parsed ${jsonList.length} native log entries');
        }
        
        // Only process new logs to avoid duplicates
        if (jsonList.length > _lastNativeLogCount) {
          final newLogs = jsonList.skip(_lastNativeLogCount).toList();
          _lastLogRefreshStatus = 'Added ${newLogs.length} new logs (${jsonList.length} total)';
          
          if (kDebugMode) {
            debugPrint('🆕 Processing ${newLogs.length} new logs');
          }
          
          for (final logData in newLogs) {
            try {
              final logEntry = LogEntry.fromJson(logData);
              final displayLog = logEntry.toDisplayString();
              
              // Insert native logs in chronological order without local timestamp prefix
              _logs.add(displayLog);
              
              if (kDebugMode) {
                debugPrint('📋 Added native log: ${logEntry.message}');
              }
            } catch (e) {
              _lastLogRefreshStatus = 'Error parsing log entry: $e';
              if (kDebugMode) {
                debugPrint('❌ Failed to parse log entry: $e');
                debugPrint('❌ Raw log data: $logData');
              }
            }
          }
          
          _lastNativeLogCount = jsonList.length;
          notifyListeners();
        } else {
          _lastLogRefreshStatus = 'No new logs (${jsonList.length} total, last processed: $_lastNativeLogCount)';
          if (kDebugMode) {
            debugPrint('📋 No new logs to process');
          }
        }
      } else {
        _lastLogRefreshStatus = 'Native logs empty or "[]"';
        if (kDebugMode) {
          debugPrint('📋 Native logs are empty or return "[]"');
        }
      }
    } catch (e) {
      _lastLogRefreshStatus = 'Error: $e';
      if (kDebugMode) {
        debugPrint('❌ Failed to refresh native logs: $e');
      }
    }
  }
  
  @override
  void dispose() {
    // Avoid calling async shutdown() during disposal as it can cause issues
    try {
      if (_isInitialized) {
        _isInitialized = false;
        // Let the native bridge cleanup in its own time
        if (kDebugMode) {
          debugPrint('🔄 CIToolProvider disposed, marked as not initialized');
        }
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('❌ Error during CIToolProvider disposal: $e');
      }
    }
    super.dispose();
  }
}