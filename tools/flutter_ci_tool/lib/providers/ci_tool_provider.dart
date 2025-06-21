import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import '../native/midi_ci_bridge.dart';

class CIToolProvider extends ChangeNotifier {
  static const MethodChannel _channel = MethodChannel('midi_ci_tool/repository');
  
  bool _isInitialized = false;
  String _configPath = '';
  List<String> _logs = [];
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
      
      final result = await _channel.invokeMethod<bool>('initialize') ?? false;
      _isInitialized = result;
      
      if (_isInitialized) {
        _setupLoggingCallback();
      } else {
        _lastError = 'Failed to initialize CIToolRepository';
      }
      
      notifyListeners();
      return _isInitialized;
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
        await _channel.invokeMethod('shutdown');
        MidiCIBridge.instance.shutdown();
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
      final result = await _channel.invokeMethod<bool>('loadConfig', {
        'path': configPath,
      }) ?? false;
      
      if (result) {
        _configPath = configPath;
        _addLog('Configuration loaded from: $configPath');
      } else {
        _lastError = 'Failed to load configuration from: $configPath';
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Load config error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> saveConfig(String configPath) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('saveConfig', {
        'path': configPath,
      }) ?? false;
      
      if (result) {
        _configPath = configPath;
        _addLog('Configuration saved to: $configPath');
      } else {
        _lastError = 'Failed to save configuration to: $configPath';
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Save config error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<void> loadConfigDefault() async {
    try {
      _lastError = null;
      await _channel.invokeMethod('loadConfigDefault');
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
      await _channel.invokeMethod('saveConfigDefault');
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
    _logs.add('[$timestamp] $message');
    notifyListeners();
  }
  
  void _setupLoggingCallback() {
  }
  
  @override
  void dispose() {
    shutdown();
    super.dispose();
  }
}
