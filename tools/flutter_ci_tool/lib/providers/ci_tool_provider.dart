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
  
  bool get isInitialized => _isInitialized;
  String get configPath => _configPath;
  List<String> get logs => List.unmodifiable(_logs);
  String? get lastError => _lastError;
  bool get libraryLoaded => _libraryLoaded;
  String? get libraryLoadError => _libraryLoadError;
  
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
      
      // Initial refresh of any existing logs
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
    await _refreshNativeLogs();
  }
  
  /// Refresh logs from native side and merge with local logs
  Future<void> _refreshNativeLogs() async {
    if (!_isInitialized) return;
    
    try {
      final nativeLogsJson = MidiCIBridge.instance.getLogsJson();
      
      if (nativeLogsJson.isNotEmpty && nativeLogsJson != '[]') {
        final List<dynamic> jsonList = json.decode(nativeLogsJson);
        
        // Only process new logs to avoid duplicates
        if (jsonList.length > _lastNativeLogCount) {
          final newLogs = jsonList.skip(_lastNativeLogCount).toList();
          
          for (final logData in newLogs) {
            try {
              final logEntry = LogEntry.fromJson(logData);
              final displayLog = logEntry.toDisplayString();
              
              // Insert native logs in chronological order without local timestamp prefix
              _logs.add(displayLog);
              
              if (kDebugMode) {
                debugPrint('📋 Native log: ${logEntry.message}');
              }
            } catch (e) {
              if (kDebugMode) {
                debugPrint('❌ Failed to parse log entry: $e');
              }
            }
          }
          
          _lastNativeLogCount = jsonList.length;
          notifyListeners();
        }
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('❌ Failed to refresh native logs: $e');
      }
    }
  }
  
  @override
  void dispose() {
    shutdown();
    super.dispose();
  }
}