import 'package:flutter/foundation.dart';
import '../state/mutable_state.dart';
import 'ci_device_model.dart';

/// Flutter equivalent of C++ SubscriptionState
class SubscriptionState {
  final String propertyId;
  final SubscriptionStateEnum state;
  
  SubscriptionState(this.propertyId, this.state);
  
  @override
  bool operator ==(Object other) =>
      identical(this, other) ||
      other is SubscriptionState &&
          runtimeType == other.runtimeType &&
          propertyId == other.propertyId &&
          state == other.state;

  @override
  int get hashCode => propertyId.hashCode ^ state.hashCode;
}

enum SubscriptionStateEnum {
  subscribing,
  subscribed,
  unsubscribed,
}

/// Flutter equivalent of C++ ClientConnectionModel
/// Manages reactive state for a client connection including profiles, properties, and device info
class ClientConnectionModel extends ChangeNotifier {
  final Connection _connection;
  
  // Reactive state lists - mirror C++ MutableStateList usage
  final MutableStateList<Profile> _profiles = MutableStateList<Profile>();
  final MutableStateList<SubscriptionState> _subscriptions = MutableStateList<SubscriptionState>();
  final MutableStateList<Property> _properties = MutableStateList<Property>();
  
  // Reactive state - mirror C++ MutableState usage
  final MutableState<String> _deviceInfo = MutableState<String>('');
  
  // Callbacks - mirror C++ callback system
  final List<VoidCallback> _profilesChangedCallbacks = [];
  final List<VoidCallback> _propertiesChangedCallbacks = [];
  final List<VoidCallback> _deviceInfoChangedCallbacks = [];
  
  ClientConnectionModel(this._connection) {
    _setupListeners();
    _initializeFromConnection();
  }
  
  /// Get the underlying connection
  Connection get connection => _connection;
  
  /// Get reactive profiles list
  MutableStateList<Profile> get profiles => _profiles;
  
  /// Get reactive subscriptions list
  MutableStateList<SubscriptionState> get subscriptions => _subscriptions;
  
  /// Get reactive properties list
  MutableStateList<Property> get properties => _properties;
  
  /// Get reactive device info
  MutableState<String> get deviceInfo => _deviceInfo;
  
  /// Get current device info value
  String getDeviceInfoValue() => _deviceInfo.get();
  
  void _setupListeners() {
    // Setup reactive listeners to trigger callbacks when data changes
    _profiles.setCollectionChangedHandler((action, item) {
      _onProfileChanged();
    });
    
    _properties.setCollectionChangedHandler((action, item) {
      _onPropertyValueUpdated();
    });
    
    _deviceInfo.setValueChangedHandler(() {
      _onDeviceInfoChanged();
    });
  }
  
  void _initializeFromConnection() {
    // Initialize reactive state from connection data
    _deviceInfo.set(_connection.deviceInfo);
    
    for (final profile in _connection.profiles) {
      _profiles.add(profile);
    }
    
    for (final property in _connection.properties) {
      _properties.add(property);
    }
  }
  
  /// Set profile state - mirrors C++ setProfile method
  void setProfile(int group, int address, String profileId, 
                 bool newEnabled, int newNumChannelsRequested) {
    // Find and update profile
    final profileIndex = _profiles.items.indexWhere((p) => 
        p.group == group && p.address == address && p.profileId == profileId);
    
    if (profileIndex >= 0) {
      final oldProfile = _profiles.items[profileIndex];
      final newProfile = Profile(
        profileId: profileId,
        group: group,
        address: address,
        enabled: newEnabled,
        numChannelsRequested: newNumChannelsRequested,
      );
      
      _profiles.remove(oldProfile);
      _profiles.add(newProfile);
    }
  }
  
  /// Get property data - mirrors C++ getPropertyData method
  void getPropertyData(String resource, {String encoding = '', 
                      int paginateOffset = -1, int paginateLimit = -1}) {
    // TODO: Implement via FFI call to native side
    debugPrint('getPropertyData: $resource');
  }
  
  /// Set property data - mirrors C++ setPropertyData method
  void setPropertyData(String resource, String resId, List<int> data, 
                      {String encoding = '', bool isPartial = false}) {
    // Update property in reactive list
    final propertyIndex = _properties.items.indexWhere((p) => 
        p.resourceId == resId);
    
    if (propertyIndex >= 0) {
      final oldProperty = _properties.items[propertyIndex];
      final newProperty = Property(
        propertyId: oldProperty.propertyId,
        resourceId: resId,
        name: oldProperty.name,
        mediaType: oldProperty.mediaType,
        data: data,
      );
      
      _properties.remove(oldProperty);
      _properties.add(newProperty);
    }
  }
  
  /// Subscribe to property - mirrors C++ subscribeProperty method
  void subscribeProperty(String resource, {String mutualEncoding = ''}) {
    final subscription = SubscriptionState(resource, SubscriptionStateEnum.subscribing);
    _subscriptions.add(subscription);
    
    // TODO: Implement via FFI call to native side
    debugPrint('subscribeProperty: $resource');
  }
  
  /// Unsubscribe from property - mirrors C++ unsubscribeProperty method
  void unsubscribeProperty(String resource) {
    _subscriptions.removeWhere((s) => s.propertyId == resource);
    
    // TODO: Implement via FFI call to native side
    debugPrint('unsubscribeProperty: $resource');
  }
  
  /// Request MIDI message report - mirrors C++ requestMidiMessageReport method
  void requestMidiMessageReport(int address, int targetMuid, 
                               {int messageDataControl = 0xFF,
                                int systemMessages = 0xFF,
                                int channelControllerMessages = 0xFF,
                                int noteDataMessages = 0xFF}) {
    // TODO: Implement via FFI call to native side
    debugPrint('requestMidiMessageReport: address=$address, targetMuid=$targetMuid');
  }
  
  // Callback management - mirrors C++ callback system
  void addProfilesChangedCallback(VoidCallback callback) {
    _profilesChangedCallbacks.add(callback);
  }
  
  void addPropertiesChangedCallback(VoidCallback callback) {
    _propertiesChangedCallbacks.add(callback);
  }
  
  void addDeviceInfoChangedCallback(VoidCallback callback) {
    _deviceInfoChangedCallbacks.add(callback);
  }
  
  void removeProfilesChangedCallback(VoidCallback callback) {
    _profilesChangedCallbacks.remove(callback);
  }
  
  void removePropertiesChangedCallback(VoidCallback callback) {
    _propertiesChangedCallbacks.remove(callback);
  }
  
  void removeDeviceInfoChangedCallback(VoidCallback callback) {
    _deviceInfoChangedCallbacks.remove(callback);
  }
  
  void _onProfileChanged() {
    for (final callback in _profilesChangedCallbacks) {
      callback();
    }
  }
  
  void _onPropertyValueUpdated() {
    for (final callback in _propertiesChangedCallbacks) {
      callback();
    }
  }
  
  void _onDeviceInfoChanged() {
    for (final callback in _deviceInfoChangedCallbacks) {
      callback();
    }
  }
  
  @override
  void dispose() {
    _profiles.dispose();
    _subscriptions.dispose();
    _properties.dispose();
    _deviceInfo.dispose();
    _profilesChangedCallbacks.clear();
    _propertiesChangedCallbacks.clear();  
    _deviceInfoChangedCallbacks.clear();
    super.dispose();
  }
}