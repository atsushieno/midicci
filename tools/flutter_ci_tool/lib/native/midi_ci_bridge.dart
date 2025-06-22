import 'dart:ffi';
import 'dart:convert';
import '../ffi/midicci_bindings.dart';

class MidiCIBridge {
  static MidiCIBridge? _instance;
  static MidiCIBridge get instance => _instance ??= MidiCIBridge._();

  late final MidiCCIBindings _bindings;
  Pointer<Void>? _repositoryHandle;
  Pointer<Void>? _deviceManagerHandle;
  Pointer<Void>? _deviceModelHandle;
  Pointer<Void>? _midiManagerHandle;

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
    await _ensureDeviceModel();
    if (_deviceModelHandle != null) {
      try {
        _bindings.sendDiscovery(_deviceModelHandle!);
      } catch (e) {
        // Handle error
      }
    }
  }

  /// Get connections as JSON list
  Future<List<Map<String, dynamic>>> getConnections() async {
    await _ensureDeviceModel();
    if (_deviceModelHandle != null) {
      try {
        final jsonString = _bindings.getConnectionsJson(_deviceModelHandle!);
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
    await _ensureMidiManager();
    if (_midiManagerHandle != null) {
      try {
        final jsonString = _bindings.getInputDevicesJson(_midiManagerHandle!);
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
    await _ensureMidiManager();
    if (_midiManagerHandle != null) {
      try {
        final jsonString = _bindings.getOutputDevicesJson(_midiManagerHandle!);
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
    await _ensureMidiManager();
    if (_midiManagerHandle != null) {
      try {
        return _bindings.setInputDevice(_midiManagerHandle!, deviceId);
      } catch (e) {
        return false;
      }
    }
    return false;
  }

  /// Set output device
  Future<bool> setOutputDevice(String deviceId) async {
    await _ensureMidiManager();
    if (_midiManagerHandle != null) {
      try {
        return _bindings.setOutputDevice(_midiManagerHandle!, deviceId);
      } catch (e) {
        return false;
      }
    }
    return false;
  }

  /// Ensure device manager and model handles are initialized
  Future<void> _ensureDeviceModel() async {
    if (_deviceManagerHandle == null && _repositoryHandle != null) {
      _deviceManagerHandle = _bindings.getDeviceManager(_repositoryHandle!);
    }
    if (_deviceModelHandle == null && _deviceManagerHandle != null) {
      _deviceModelHandle = _bindings.getDeviceModel(_deviceManagerHandle!);
    }
  }

  /// Ensure MIDI manager handle is initialized
  Future<void> _ensureMidiManager() async {
    if (_midiManagerHandle == null && _repositoryHandle != null) {
      _midiManagerHandle = _bindings.getMidiDeviceManager(_repositoryHandle!);
    }
  }
}