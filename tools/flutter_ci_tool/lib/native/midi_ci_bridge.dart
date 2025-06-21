import 'package:flutter/services.dart';

class MidiCIBridge {
  static const MethodChannel _channel = MethodChannel('midi_ci_tool/bridge');
  
  static MidiCIBridge? _instance;
  static MidiCIBridge get instance => _instance ??= MidiCIBridge._();
  
  MidiCIBridge._();
  
  Future<bool> initialize() async {
    try {
      final result = await _channel.invokeMethod<bool>('initialize') ?? false;
      return result;
    } catch (e) {
      return false;
    }
  }
  
  void shutdown() {
    try {
      _channel.invokeMethod('shutdown');
    } catch (e) {
    }
  }
}
