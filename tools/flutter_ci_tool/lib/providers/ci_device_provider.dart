import 'package:flutter/foundation.dart';
import '../models/ci_device_model.dart';

class CIDeviceProvider extends ChangeNotifier {
  final List<Connection> _connections = [];
  final List<Profile> _localProfiles = [];
  final List<Property> _localProperties = [];
  final List<MidiDevice> _inputDevices = [];
  final List<MidiDevice> _outputDevices = [];
  String? _selectedInputDevice;
  String? _selectedOutputDevice;
  String? _lastError;
  
  List<Connection> get connections => List.unmodifiable(_connections);
  List<Profile> get localProfiles => List.unmodifiable(_localProfiles);
  List<Property> get localProperties => List.unmodifiable(_localProperties);
  List<MidiDevice> get inputDevices => List.unmodifiable(_inputDevices);
  List<MidiDevice> get outputDevices => List.unmodifiable(_outputDevices);
  String? get selectedInputDevice => _selectedInputDevice;
  String? get selectedOutputDevice => _selectedOutputDevice;
  String? get lastError => _lastError;
  
  Future<void> sendDiscovery() async {
    try {
      _lastError = null;
      // TODO: Implement via FFI
      notifyListeners();
    } catch (e) {
      _lastError = 'Send discovery error: $e';
      notifyListeners();
    }
  }
  
  Future<void> refreshConnections() async {
    try {
      _lastError = null;
      // TODO: Implement via FFI
      notifyListeners();
    } catch (e) {
      _lastError = 'Refresh connections error: $e';
      notifyListeners();
    }
  }
  
  Future<void> refreshDevices() async {
    try {
      _lastError = null;
      // TODO: Implement via FFI - populate _inputDevices and _outputDevices
      notifyListeners();
    } catch (e) {
      _lastError = 'Refresh devices error: $e';
      notifyListeners();
    }
  }
  
  Future<bool> setInputDevice(String deviceId) async {
    try {
      _lastError = null;
      // TODO: Implement via FFI
      _selectedInputDevice = deviceId;
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Set input device error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> setOutputDevice(String deviceId) async {
    try {
      _lastError = null;
      // TODO: Implement via FFI
      _selectedOutputDevice = deviceId;
      notifyListeners();
      return true;
    } catch (e) {
      _lastError = 'Set output device error: $e';
      notifyListeners();
      return false;
    }
  }
}