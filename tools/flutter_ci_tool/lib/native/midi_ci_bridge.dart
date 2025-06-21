import 'dart:ffi';
import 'dart:convert';
import '../ffi/midicci_bindings.dart';

class MidiCIBridge {
  static MidiCIBridge? _instance;
  static MidiCIBridge get instance => _instance ??= MidiCIBridge._();

  late final MidiCCIBindings _bindings;
  Pointer<Void>? _repositoryHandle;

  MidiCIBridge._() {
    _bindings = MidiCCIBindings.instance;
  }

  bool get isInitialized => _repositoryHandle != null;

  Future<bool> initialize() async {
    try {
      if (_repositoryHandle != null) {
        return true; // Already initialized
      }

      _repositoryHandle = _bindings.createRepository();
      if (_repositoryHandle == nullptr) {
        _repositoryHandle = null;
        return false;
      }

      final success = _bindings.initializeRepository(_repositoryHandle!);
      if (!success) {
        _bindings.destroyRepository(_repositoryHandle!);
        _repositoryHandle = null;
        return false;
      }

      return true;
    } catch (e) {
      if (_repositoryHandle != null) {
        _bindings.destroyRepository(_repositoryHandle!);
        _repositoryHandle = null;
      }
      return false;
    }
  }

  void shutdown() {
    if (_repositoryHandle != null) {
      try {
        _bindings.shutdownRepository(_repositoryHandle!);
        _bindings.destroyRepository(_repositoryHandle!);
      } catch (e) {
        // Ignore shutdown errors
      } finally {
        _repositoryHandle = null;
      }
    }
  }

  void log(String message, {bool isOutgoing = false}) {
    if (_repositoryHandle != null) {
      try {
        _bindings.logRepository(_repositoryHandle!, message, isOutgoing);
      } catch (e) {
        // Ignore logging errors
      }
    }
  }

  String getLogsJson() {
    if (_repositoryHandle != null) {
      try {
        return _bindings.getLogsJson(_repositoryHandle!);
      } catch (e) {
        return '[]';
      }
    }
    return '[]';
  }

  int getMuid() {
    if (_repositoryHandle != null) {
      try {
        return _bindings.getMuid(_repositoryHandle!);
      } catch (e) {
        return 0;
      }
    }
    return 0;
  }

  /// Send discovery message
  Future<void> sendDiscovery() async {
    if (_repositoryHandle != null) {
      try {
        _bindings.sendDiscovery(_repositoryHandle!);
      } catch (e) {
        // Handle error
      }
    }
  }

  /// Get connections as JSON list
  Future<List<Map<String, dynamic>>> getConnections() async {
    if (_repositoryHandle != null) {
      try {
        final jsonString = _bindings.getConnectionsJson(_repositoryHandle!);
        final List<dynamic> jsonList = json.decode(jsonString);
        return jsonList.cast<Map<String, dynamic>>();
      } catch (e) {
        return [];
      }
    }
    return [];
  }

  /// Get input devices as JSON list
  Future<List<Map<String, dynamic>>> getInputDevices() async {
    if (_repositoryHandle != null) {
      try {
        final jsonString = _bindings.getInputDevicesJson(_repositoryHandle!);
        final List<dynamic> jsonList = json.decode(jsonString);
        return jsonList.cast<Map<String, dynamic>>();
      } catch (e) {
        return [];
      }
    }
    return [];
  }

  /// Get output devices as JSON list  
  Future<List<Map<String, dynamic>>> getOutputDevices() async {
    if (_repositoryHandle != null) {
      try {
        final jsonString = _bindings.getOutputDevicesJson(_repositoryHandle!);
        final List<dynamic> jsonList = json.decode(jsonString);
        return jsonList.cast<Map<String, dynamic>>();
      } catch (e) {
        return [];
      }
    }
    return [];
  }

  /// Set input device
  Future<bool> setInputDevice(String deviceId) async {
    if (_repositoryHandle != null) {
      try {
        return _bindings.setInputDevice(_repositoryHandle!, deviceId);
      } catch (e) {
        return false;
      }
    }
    return false;
  }

  /// Set output device
  Future<bool> setOutputDevice(String deviceId) async {
    if (_repositoryHandle != null) {
      try {
        return _bindings.setOutputDevice(_repositoryHandle!, deviceId);
      } catch (e) {
        return false;
      }
    }
    return false;
  }
}