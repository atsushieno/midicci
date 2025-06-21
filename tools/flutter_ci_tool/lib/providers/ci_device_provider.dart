import 'package:flutter/foundation.dart';
import '../models/ci_device_model.dart';
import '../models/client_connection_model.dart';
import '../state/mutable_state.dart';
import '../native/midi_ci_bridge.dart';

/// Flutter equivalent of C++ CIDeviceModel
/// Manages reactive state for MIDI-CI device connections, profiles, and properties
class CIDeviceProvider extends ChangeNotifier {
  // Reactive state - mirrors C++ CIDeviceModel's MutableStateList usage
  final MutableStateList<ClientConnectionModel> _connections = MutableStateList<ClientConnectionModel>();
  final MutableStateList<Profile> _localProfiles = MutableStateList<Profile>();
  final MutableStateList<Property> _localProperties = MutableStateList<Property>();
  final MutableStateList<MidiDevice> _inputDevices = MutableStateList<MidiDevice>();
  final MutableStateList<MidiDevice> _outputDevices = MutableStateList<MidiDevice>();
  
  final MutableState<String?> _selectedInputDevice = MutableState<String?>(null);
  final MutableState<String?> _selectedOutputDevice = MutableState<String?>(null);
  final MutableState<String?> _lastError = MutableState<String?>(null);
  
  // Callbacks - mirror C++ callback system
  final List<VoidCallback> _connectionsChangedCallbacks = [];
  final List<VoidCallback> _profilesUpdatedCallbacks = [];
  final List<VoidCallback> _propertiesUpdatedCallbacks = [];
  
  CIDeviceProvider() {
    _setupEventListeners();
  }
  
  // Public reactive getters - mirror C++ const getter pattern
  MutableStateList<ClientConnectionModel> get connections => _connections;
  MutableStateList<Profile> get localProfiles => _localProfiles;
  MutableStateList<Property> get localProperties => _localProperties;
  MutableStateList<MidiDevice> get inputDevices => _inputDevices;
  MutableStateList<MidiDevice> get outputDevices => _outputDevices;
  MutableState<String?> get selectedInputDevice => _selectedInputDevice;
  MutableState<String?> get selectedOutputDevice => _selectedOutputDevice;
  MutableState<String?> get lastError => _lastError;
  
  void _setupEventListeners() {
    // Setup reactive listeners - mirror C++ setup_event_listeners
    _connections.setCollectionChangedHandler((action, item) {
      _onConnectionsChanged();
    });
    
    _localProfiles.setCollectionChangedHandler((action, item) {
      _onProfilesUpdated();
    });
    
    _localProperties.setCollectionChangedHandler((action, item) {
      _onPropertiesUpdated();
    });
  }
  
  /// Send discovery message - mirrors C++ send_discovery method
  Future<void> sendDiscovery() async {
    try {
      _lastError.set(null);
      // Call native implementation via FFI
      await MidiCIBridge.instance.sendDiscovery();
      notifyListeners();
    } catch (e) {
      _lastError.set('Send discovery error: $e');
      notifyListeners();
    }
  }
  
  /// Refresh connections from native side - mirrors C++ connection management
  Future<void> refreshConnections() async {
    try {
      _lastError.set(null);
      // Get connections from native side via FFI
      final connectionsJson = await MidiCIBridge.instance.getConnections();
      
      // Parse and update reactive state
      _connections.clear();
      for (final connectionData in connectionsJson) {
        final connection = Connection.fromJson(connectionData);
        final connectionModel = ClientConnectionModel(connection);
        _connections.add(connectionModel);
      }
      
      notifyListeners();
    } catch (e) {
      _lastError.set('Refresh connections error: $e');
      notifyListeners();
    }
  }
  
  /// Refresh MIDI devices from native side
  Future<void> refreshDevices() async {
    try {
      _lastError.set(null);
      
      // Get devices from native side via FFI
      final inputDevicesJson = await MidiCIBridge.instance.getInputDevices();
      final outputDevicesJson = await MidiCIBridge.instance.getOutputDevices();
      
      // Update reactive state
      _inputDevices.clear();
      _outputDevices.clear();
      
      for (final deviceData in inputDevicesJson) {
        final device = MidiDevice.fromJson(deviceData);
        _inputDevices.add(device);
      }
      
      for (final deviceData in outputDevicesJson) {
        final device = MidiDevice.fromJson(deviceData);
        _outputDevices.add(device);
      }
      
      notifyListeners();
    } catch (e) {
      _lastError.set('Refresh devices error: $e');
      notifyListeners();
    }
  }
  
  /// Set input device - mirrors C++ device selection
  Future<bool> setInputDevice(String deviceId) async {
    try {
      _lastError.set(null);
      final success = await MidiCIBridge.instance.setInputDevice(deviceId);
      if (success) {
        _selectedInputDevice.set(deviceId);
      }
      notifyListeners();
      return success;
    } catch (e) {
      _lastError.set('Set input device error: $e');
      notifyListeners();
      return false;
    }
  }
  
  /// Set output device - mirrors C++ device selection
  Future<bool> setOutputDevice(String deviceId) async {
    try {
      _lastError.set(null);
      final success = await MidiCIBridge.instance.setOutputDevice(deviceId);
      if (success) {
        _selectedOutputDevice.set(deviceId);
      }
      notifyListeners();
      return success;
    } catch (e) {
      _lastError.set('Set output device error: $e');
      notifyListeners();
      return false;
    }
  }
  
  /// Add local profile - mirrors C++ add_local_profile
  void addLocalProfile(Profile profile) {
    _localProfiles.add(profile);
  }
  
  /// Remove local profile - mirrors C++ remove_local_profile  
  void removeLocalProfile(int group, int address, String profileId) {
    _localProfiles.removeWhere((p) => 
        p.group == group && p.address == address && p.profileId == profileId);
  }
  
  /// Add local property - mirrors C++ add_local_property
  void addLocalProperty(Property property) {
    _localProperties.add(property);
  }
  
  /// Remove local property - mirrors C++ remove_local_property
  void removeLocalProperty(String propertyId) {
    _localProperties.removeWhere((p) => p.propertyId == propertyId);
  }
  
  /// Update property value - mirrors C++ update_property_value
  void updatePropertyValue(String propertyId, String resId, List<int> data) {
    final propertyIndex = _localProperties.items.indexWhere((p) => p.propertyId == propertyId);
    if (propertyIndex >= 0) {
      final oldProperty = _localProperties.items[propertyIndex];
      final newProperty = Property(
        propertyId: propertyId,
        resourceId: resId,
        name: oldProperty.name,
        mediaType: oldProperty.mediaType,
        data: data,
      );
      _localProperties.remove(oldProperty);
      _localProperties.add(newProperty);
    }
  }
  
  // Callback management - mirrors C++ callback system
  void addConnectionsChangedCallback(VoidCallback callback) {
    _connectionsChangedCallbacks.add(callback);
  }
  
  void addProfilesUpdatedCallback(VoidCallback callback) {
    _profilesUpdatedCallbacks.add(callback);
  }
  
  void addPropertiesUpdatedCallback(VoidCallback callback) {
    _propertiesUpdatedCallbacks.add(callback);
  }
  
  void removeConnectionsChangedCallback(VoidCallback callback) {
    _connectionsChangedCallbacks.remove(callback);
  }
  
  void removeProfilesUpdatedCallback(VoidCallback callback) {
    _profilesUpdatedCallbacks.remove(callback);
  }
  
  void removePropertiesUpdatedCallback(VoidCallback callback) {
    _propertiesUpdatedCallbacks.remove(callback);
  }
  
  void _onConnectionsChanged() {
    for (final callback in _connectionsChangedCallbacks) {
      callback();
    }
  }
  
  void _onProfilesUpdated() {
    for (final callback in _profilesUpdatedCallbacks) {
      callback();
    }
  }
  
  void _onPropertiesUpdated() {
    for (final callback in _propertiesUpdatedCallbacks) {
      callback();
    }
  }
  
  @override
  void dispose() {
    _connections.dispose();
    _localProfiles.dispose();
    _localProperties.dispose();
    _inputDevices.dispose();
    _outputDevices.dispose();
    _selectedInputDevice.dispose();
    _selectedOutputDevice.dispose();
    _lastError.dispose();
    _connectionsChangedCallbacks.clear();
    _profilesUpdatedCallbacks.clear();
    _propertiesUpdatedCallbacks.clear();
    super.dispose();
  }
}