import 'dart:ffi';
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
}