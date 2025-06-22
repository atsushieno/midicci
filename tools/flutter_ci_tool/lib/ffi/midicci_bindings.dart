import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';
import 'package:flutter/foundation.dart';

// Native function signatures
typedef CIToolRepositoryCreateNative = Pointer<Void> Function();
typedef CIToolRepositoryCreateDart = Pointer<Void> Function();

typedef CIToolRepositoryDestroyNative = Void Function(Pointer<Void>);
typedef CIToolRepositoryDestroyDart = void Function(Pointer<Void>);

typedef CIToolRepositoryInitializeNative = Bool Function(Pointer<Void>);
typedef CIToolRepositoryInitializeDart = bool Function(Pointer<Void>);

typedef CIToolRepositoryShutdownNative = Void Function(Pointer<Void>);
typedef CIToolRepositoryShutdownDart = void Function(Pointer<Void>);

typedef CIToolRepositoryLogNative = Void Function(Pointer<Void>, Pointer<Utf8>, Bool);
typedef CIToolRepositoryLogDart = void Function(Pointer<Void>, Pointer<Utf8>, bool);

typedef CIToolRepositoryGetLogsJsonNative = Pointer<Utf8> Function(Pointer<Void>);
typedef CIToolRepositoryGetLogsJsonDart = Pointer<Utf8> Function(Pointer<Void>);

typedef CIToolRepositoryGetMuidNative = Uint32 Function(Pointer<Void>);
typedef CIToolRepositoryGetMuidDart = int Function(Pointer<Void>);

// Device Manager Functions
typedef CIDeviceManagerSendDiscoveryNative = Void Function(Pointer<Void>);
typedef CIDeviceManagerSendDiscoveryDart = void Function(Pointer<Void>);

typedef CIDeviceManagerGetConnectionsJsonNative = Pointer<Utf8> Function(Pointer<Void>);
typedef CIDeviceManagerGetConnectionsJsonDart = Pointer<Utf8> Function(Pointer<Void>);

// MIDI Device Manager Functions
typedef MidiDeviceManagerGetInputDevicesJsonNative = Pointer<Utf8> Function(Pointer<Void>);
typedef MidiDeviceManagerGetInputDevicesJsonDart = Pointer<Utf8> Function(Pointer<Void>);

typedef MidiDeviceManagerGetOutputDevicesJsonNative = Pointer<Utf8> Function(Pointer<Void>);
typedef MidiDeviceManagerGetOutputDevicesJsonDart = Pointer<Utf8> Function(Pointer<Void>);

typedef MidiDeviceManagerSetInputDeviceNative = Bool Function(Pointer<Void>, Pointer<Utf8>);
typedef MidiDeviceManagerSetInputDeviceDart = bool Function(Pointer<Void>, Pointer<Utf8>);

typedef MidiDeviceManagerSetOutputDeviceNative = Bool Function(Pointer<Void>, Pointer<Utf8>);
typedef MidiDeviceManagerSetOutputDeviceDart = bool Function(Pointer<Void>, Pointer<Utf8>);

// Repository to Manager Functions
typedef CIToolRepositoryGetDeviceManagerNative = Pointer<Void> Function(Pointer<Void>);
typedef CIToolRepositoryGetDeviceManagerDart = Pointer<Void> Function(Pointer<Void>);

typedef CIToolRepositoryGetMidiDeviceManagerNative = Pointer<Void> Function(Pointer<Void>);
typedef CIToolRepositoryGetMidiDeviceManagerDart = Pointer<Void> Function(Pointer<Void>);

typedef CIDeviceManagerGetDeviceModelNative = Pointer<Void> Function(Pointer<Void>);
typedef CIDeviceManagerGetDeviceModelDart = Pointer<Void> Function(Pointer<Void>);

// Log Callback Functions
typedef LogCallbackNative = Void Function(Pointer<Utf8>, Bool, Pointer<Utf8>);
typedef LogCallbackDart = void Function(Pointer<Utf8>, bool, Pointer<Utf8>);

typedef CIToolRepositorySetLogCallbackNative = Void Function(Pointer<Void>, Pointer<NativeFunction<LogCallbackNative>>);
typedef CIToolRepositorySetLogCallbackDart = void Function(Pointer<Void>, Pointer<NativeFunction<LogCallbackNative>>);

typedef CIToolRepositoryClearLogsNative = Void Function(Pointer<Void>);
typedef CIToolRepositoryClearLogsDart = void Function(Pointer<Void>);

class MidiCCIBindings {
  static MidiCCIBindings? _instance;
  static MidiCCIBindings get instance => _instance ??= MidiCCIBindings._();

  late final DynamicLibrary _dylib;
  late final CIToolRepositoryCreateDart _createRepository;
  late final CIToolRepositoryDestroyDart _destroyRepository;
  late final CIToolRepositoryInitializeDart _initializeRepository;
  late final CIToolRepositoryShutdownDart _shutdownRepository;
  late final CIToolRepositoryLogDart _logRepository;
  late final CIToolRepositoryGetLogsJsonDart _getLogsJson;
  late final CIToolRepositoryGetMuidDart _getMuid;
  
  // Device Manager Functions
  late final CIDeviceManagerSendDiscoveryDart _sendDiscovery;
  late final CIDeviceManagerGetConnectionsJsonDart _getConnectionsJson;
  
  // MIDI Device Manager Functions
  late final MidiDeviceManagerGetInputDevicesJsonDart _getInputDevicesJson;
  late final MidiDeviceManagerGetOutputDevicesJsonDart _getOutputDevicesJson;
  late final MidiDeviceManagerSetInputDeviceDart _setInputDevice;
  late final MidiDeviceManagerSetOutputDeviceDart _setOutputDevice;
  
  // Repository to Manager Functions
  late final CIToolRepositoryGetDeviceManagerDart _getDeviceManager;
  late final CIToolRepositoryGetMidiDeviceManagerDart _getMidiDeviceManager;
  late final CIDeviceManagerGetDeviceModelDart _getDeviceModel;
  
  // Log Callback Functions
  late final CIToolRepositorySetLogCallbackDart _setLogCallback;
  late final CIToolRepositoryClearLogsDart _clearLogs;

  MidiCCIBindings._() {
    _loadLibrary();
    _bindFunctions();
  }

  void _loadLibrary() {
    String libraryName;
    if (Platform.isMacOS) {
      libraryName = 'libmidicci-flutter-wrapper.dylib';
    } else if (Platform.isLinux) {
      libraryName = 'libmidicci-flutter-wrapper.so';
    } else if (Platform.isWindows) {
      libraryName = 'midicci-flutter-wrapper.dll';
    } else {
      throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
    }
    
    try {
      _dylib = DynamicLibrary.open(libraryName);
      if (kDebugMode) {
        debugPrint('✅ Successfully loaded native library: $libraryName');
      }
    } catch (e) {
      if (kDebugMode) {
        debugPrint('❌ Failed to load native library: $libraryName');
        debugPrint('Error details: $e');
        debugPrint('Current working directory: ${Directory.current.path}');
        debugPrint('Platform: ${Platform.operatingSystem}');
        
        // List files in current directory to help debug
        try {
          final files = Directory.current.listSync()
              .where((f) => f.path.contains('.dylib') || f.path.contains('.so') || f.path.contains('.dll'))
              .map((f) => f.path)
              .toList();
          debugPrint('Native libraries found in current directory: $files');
        } catch (_) {
          debugPrint('Could not list current directory contents');
        }
      }
      
      rethrow;
    }
  }

  void _bindFunctions() {
    _createRepository = _dylib
        .lookup<NativeFunction<CIToolRepositoryCreateNative>>('ci_tool_repository_create')
        .asFunction();

    _destroyRepository = _dylib
        .lookup<NativeFunction<CIToolRepositoryDestroyNative>>('ci_tool_repository_destroy')
        .asFunction();

    _initializeRepository = _dylib
        .lookup<NativeFunction<CIToolRepositoryInitializeNative>>('ci_tool_repository_initialize')
        .asFunction();

    _shutdownRepository = _dylib
        .lookup<NativeFunction<CIToolRepositoryShutdownNative>>('ci_tool_repository_shutdown')
        .asFunction();

    _logRepository = _dylib
        .lookup<NativeFunction<CIToolRepositoryLogNative>>('ci_tool_repository_log')
        .asFunction();

    _getLogsJson = _dylib
        .lookup<NativeFunction<CIToolRepositoryGetLogsJsonNative>>('ci_tool_repository_get_logs_json')
        .asFunction();

    _getMuid = _dylib
        .lookup<NativeFunction<CIToolRepositoryGetMuidNative>>('ci_tool_repository_get_muid')
        .asFunction();

    // Bind device manager functions
    _sendDiscovery = _dylib
        .lookup<NativeFunction<CIDeviceManagerSendDiscoveryNative>>('ci_device_model_send_discovery')
        .asFunction();

    _getConnectionsJson = _dylib
        .lookup<NativeFunction<CIDeviceManagerGetConnectionsJsonNative>>('ci_device_model_get_connections_json')
        .asFunction();

    // Bind MIDI device manager functions
    _getInputDevicesJson = _dylib
        .lookup<NativeFunction<MidiDeviceManagerGetInputDevicesJsonNative>>('midi_device_manager_get_input_devices_json')
        .asFunction();

    _getOutputDevicesJson = _dylib
        .lookup<NativeFunction<MidiDeviceManagerGetOutputDevicesJsonNative>>('midi_device_manager_get_output_devices_json')
        .asFunction();

    _setInputDevice = _dylib
        .lookup<NativeFunction<MidiDeviceManagerSetInputDeviceNative>>('midi_device_manager_set_input_device')
        .asFunction();

    _setOutputDevice = _dylib
        .lookup<NativeFunction<MidiDeviceManagerSetOutputDeviceNative>>('midi_device_manager_set_output_device')
        .asFunction();

    // Bind repository to manager functions
    _getDeviceManager = _dylib
        .lookup<NativeFunction<CIToolRepositoryGetDeviceManagerNative>>('ci_tool_repository_get_device_manager')
        .asFunction();

    _getMidiDeviceManager = _dylib
        .lookup<NativeFunction<CIToolRepositoryGetMidiDeviceManagerNative>>('ci_tool_repository_get_midi_device_manager')
        .asFunction();

    _getDeviceModel = _dylib
        .lookup<NativeFunction<CIDeviceManagerGetDeviceModelNative>>('ci_device_manager_get_device_model')
        .asFunction();

    // Bind log callback functions
    _setLogCallback = _dylib
        .lookup<NativeFunction<CIToolRepositorySetLogCallbackNative>>('ci_tool_repository_set_log_callback')
        .asFunction();

    _clearLogs = _dylib
        .lookup<NativeFunction<CIToolRepositoryClearLogsNative>>('ci_tool_repository_clear_logs')
        .asFunction();
  }

  Pointer<Void> createRepository() => _createRepository();
  
  void destroyRepository(Pointer<Void> handle) => _destroyRepository(handle);
  
  bool initializeRepository(Pointer<Void> handle) => _initializeRepository(handle);
  
  void shutdownRepository(Pointer<Void> handle) => _shutdownRepository(handle);
  
  void logRepository(Pointer<Void> handle, String message, bool isOutgoing) {
    final messagePtr = message.toNativeUtf8();
    try {
      _logRepository(handle, messagePtr, isOutgoing);
    } finally {
      malloc.free(messagePtr);
    }
  }
  
  String getLogsJson(Pointer<Void> handle) {
    final resultPtr = _getLogsJson(handle);
    if (resultPtr == nullptr) return '[]';
    try {
      return resultPtr.toDartString();
    } finally {
      // Note: Don't free this as it's managed by the C++ side
    }
  }
  
  int getMuid(Pointer<Void> handle) => _getMuid(handle);
  
  // Device Manager Methods
  void sendDiscovery(Pointer<Void> handle) => _sendDiscovery(handle);
  
  String getConnectionsJson(Pointer<Void> handle) {
    final resultPtr = _getConnectionsJson(handle);
    if (resultPtr == nullptr) return '[]';
    try {
      return resultPtr.toDartString();
    } finally {
      // Note: Don't free this as it's managed by the C++ side
    }
  }
  
  // MIDI Device Manager Methods
  String getInputDevicesJson(Pointer<Void> handle) {
    final resultPtr = _getInputDevicesJson(handle);
    if (resultPtr == nullptr) return '[]';
    try {
      return resultPtr.toDartString();
    } finally {
      // Note: Don't free this as it's managed by the C++ side
    }
  }
  
  String getOutputDevicesJson(Pointer<Void> handle) {
    final resultPtr = _getOutputDevicesJson(handle);
    if (resultPtr == nullptr) return '[]';
    try {
      return resultPtr.toDartString();
    } finally {
      // Note: Don't free this as it's managed by the C++ side
    }
  }
  
  bool setInputDevice(Pointer<Void> handle, String deviceId) {
    final deviceIdPtr = deviceId.toNativeUtf8();
    try {
      return _setInputDevice(handle, deviceIdPtr);
    } finally {
      malloc.free(deviceIdPtr);
    }
  }
  
  bool setOutputDevice(Pointer<Void> handle, String deviceId) {
    final deviceIdPtr = deviceId.toNativeUtf8();
    try {
      return _setOutputDevice(handle, deviceIdPtr);
    } finally {
      malloc.free(deviceIdPtr);
    }
  }
  
  // Repository to Manager Methods
  Pointer<Void> getDeviceManager(Pointer<Void> repositoryHandle) {
    return _getDeviceManager(repositoryHandle);
  }
  
  Pointer<Void> getMidiDeviceManager(Pointer<Void> repositoryHandle) {
    return _getMidiDeviceManager(repositoryHandle);
  }
  
  Pointer<Void> getDeviceModel(Pointer<Void> deviceManagerHandle) {
    return _getDeviceModel(deviceManagerHandle);
  }
  
  // Log Callback Methods
  void setLogCallback(Pointer<Void> handle, Pointer<NativeFunction<LogCallbackNative>> callback) {
    _setLogCallback(handle, callback);
  }
  
  void clearLogs(Pointer<Void> handle) {
    _clearLogs(handle);
  }
}