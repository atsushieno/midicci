import 'package:flutter/foundation.dart';
import '../native/midi_ci_bridge.dart';

class CIToolProvider extends ChangeNotifier {
  bool _isInitialized = false;
  String _configPath = '';
  final List<String> _logs = [];
  String? _lastError;
  
  bool get isInitialized => _isInitialized;
  String get configPath => _configPath;
  List<String> get logs => List.unmodifiable(_logs);
  String? get lastError => _lastError;
  
  Future<bool> initialize() async {
    try {
      _lastError = null;
      
      final bridgeInitialized = await MidiCIBridge.instance.initialize();
      if (!bridgeInitialized) {
        _lastError = 'Failed to initialize native bridge';
        notifyListeners();
        return false;
      }
      
      _isInitialized = true;
      _addLog('MIDI-CI Tool initialized successfully');
      
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Initialization error: $e';
      _isInitialized = false;
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
  
  @override
  void dispose() {
    shutdown();
    super.dispose();
  }
}