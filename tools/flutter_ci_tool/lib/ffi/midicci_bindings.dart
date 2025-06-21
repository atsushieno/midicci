import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

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

  MidiCCIBindings._() {
    _loadLibrary();
    _bindFunctions();
  }

  void _loadLibrary() {
    if (Platform.isMacOS) {
      _dylib = DynamicLibrary.open('libmidicci-flutter-wrapper.dylib');
    } else if (Platform.isLinux) {
      _dylib = DynamicLibrary.open('libmidicci-flutter-wrapper.so');
    } else if (Platform.isWindows) {
      _dylib = DynamicLibrary.open('midicci-flutter-wrapper.dll');
    } else {
      throw UnsupportedError('Unsupported platform');
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
}