import 'package:flutter/foundation.dart';
import 'package:flutter/services.dart';
import '../models/ci_device_model.dart';

class CIDeviceProvider extends ChangeNotifier {
  static const MethodChannel _channel = MethodChannel('midi_ci_tool/device');
  
  List<Connection> _connections = [];
  List<Profile> _localProfiles = [];
  List<Property> _localProperties = [];
  List<MidiDevice> _inputDevices = [];
  List<MidiDevice> _outputDevices = [];
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
      await _channel.invokeMethod('sendDiscovery');
      notifyListeners();
    } catch (e) {
      _lastError = 'Send discovery error: $e';
      notifyListeners();
    }
  }
  
  Future<void> refreshConnections() async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<String>('getConnections');
      if (result != null) {
      }
      notifyListeners();
    } catch (e) {
      _lastError = 'Refresh connections error: $e';
      notifyListeners();
    }
  }
  
  Future<void> refreshDevices() async {
    try {
      _lastError = null;
      final inputResult = await _channel.invokeMethod<String>('getInputDevices');
      final outputResult = await _channel.invokeMethod<String>('getOutputDevices');
      
      if (inputResult != null) {
      }
      if (outputResult != null) {
      }
      
      notifyListeners();
    } catch (e) {
      _lastError = 'Refresh devices error: $e';
      notifyListeners();
    }
  }
  
  Future<bool> setInputDevice(String deviceId) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('setInputDevice', {
        'deviceId': deviceId,
      }) ?? false;
      
      if (result) {
        _selectedInputDevice = deviceId;
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Set input device error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> setOutputDevice(String deviceId) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('setOutputDevice', {
        'deviceId': deviceId,
      }) ?? false;
      
      if (result) {
        _selectedOutputDevice = deviceId;
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Set output device error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> addLocalProfile(Profile profile) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('addLocalProfile', {
        'profile': profile.toJson(),
      }) ?? false;
      
      if (result) {
        _localProfiles.add(profile);
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Add local profile error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> removeLocalProfile(Profile profile) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('removeLocalProfile', {
        'group': profile.group,
        'address': profile.address,
        'profileId': profile.profileId,
      }) ?? false;
      
      if (result) {
        _localProfiles.remove(profile);
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Remove local profile error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> addLocalProperty(Property property) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('addLocalProperty', {
        'property': property.toJson(),
      }) ?? false;
      
      if (result) {
        _localProperties.add(property);
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Add local property error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> removeLocalProperty(Property property) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('removeLocalProperty', {
        'propertyId': property.propertyId,
      }) ?? false;
      
      if (result) {
        _localProperties.remove(property);
      }
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Remove local property error: $e';
      notifyListeners();
      return false;
    }
  }
  
  Future<bool> updatePropertyValue(String propertyId, String resourceId, List<int> data) async {
    try {
      _lastError = null;
      final result = await _channel.invokeMethod<bool>('updatePropertyValue', {
        'propertyId': propertyId,
        'resourceId': resourceId,
        'data': data,
      }) ?? false;
      
      notifyListeners();
      return result;
    } catch (e) {
      _lastError = 'Update property value error: $e';
      notifyListeners();
      return false;
    }
  }
}
