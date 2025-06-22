import 'package:json_annotation/json_annotation.dart';

part 'ci_device_model.g.dart';

@JsonSerializable()
class Connection {
  final int targetMuid;
  final String deviceInfo;
  final bool isConnected;
  final List<Profile> profiles;
  final List<Property> properties;

  Connection({
    required this.targetMuid,
    required this.deviceInfo,
    required this.isConnected,
    required this.profiles,
    required this.properties,
  });

  factory Connection.fromJson(Map<String, dynamic> json) => _$ConnectionFromJson(json);
  Map<String, dynamic> toJson() => _$ConnectionToJson(this);
}

@JsonSerializable()
class Profile {
  final String profileId;
  final int group;
  final int address;
  final bool enabled;
  final int numChannelsRequested;

  Profile({
    required this.profileId,
    required this.group,
    required this.address,
    required this.enabled,
    required this.numChannelsRequested,
  });

  factory Profile.fromJson(Map<String, dynamic> json) => _$ProfileFromJson(json);
  Map<String, dynamic> toJson() => _$ProfileToJson(this);
}

@JsonSerializable()
class Property {
  final String propertyId;
  final String resourceId;
  final String name;
  final String mediaType;
  final List<int> data;

  Property({
    required this.propertyId,
    required this.resourceId,
    required this.name,
    required this.mediaType,
    required this.data,
  });

  factory Property.fromJson(Map<String, dynamic> json) => _$PropertyFromJson(json);
  Map<String, dynamic> toJson() => _$PropertyToJson(this);
}

@JsonSerializable()
class MidiDevice {
  final String deviceId;
  final String name;
  final bool isInput;

  MidiDevice({
    required this.deviceId,
    required this.name,
    required this.isInput,
  });

  factory MidiDevice.fromJson(Map<String, dynamic> json) => _$MidiDeviceFromJson(json);
  Map<String, dynamic> toJson() => _$MidiDeviceToJson(this);
}

@JsonSerializable()
class LogEntry {
  final String timestamp;
  final bool isOutgoing;
  final String message;

  LogEntry({
    required this.timestamp,
    required this.isOutgoing,
    required this.message,
  });

  factory LogEntry.fromJson(Map<String, dynamic> json) => _$LogEntryFromJson(json);
  Map<String, dynamic> toJson() => _$LogEntryToJson(this);
  
  /// Format the log entry for display in the UI
  String toDisplayString() {
    final direction = isOutgoing ? 'OUT' : 'IN';
    final time = DateTime.parse(timestamp);
    final timeStr = '${time.hour.toString().padLeft(2, '0')}:'
                   '${time.minute.toString().padLeft(2, '0')}:'
                   '${time.second.toString().padLeft(2, '0')}';
    return '[$timeStr] $direction: $message';
  }
}
