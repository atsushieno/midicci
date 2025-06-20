// GENERATED CODE - DO NOT MODIFY BY HAND

part of 'ci_device_model.dart';

// **************************************************************************
// JsonSerializableGenerator
// **************************************************************************

Connection _$ConnectionFromJson(Map<String, dynamic> json) => Connection(
      targetMuid: (json['targetMuid'] as num).toInt(),
      deviceInfo: json['deviceInfo'] as String,
      isConnected: json['isConnected'] as bool,
      profiles: (json['profiles'] as List<dynamic>)
          .map((e) => Profile.fromJson(e as Map<String, dynamic>))
          .toList(),
      properties: (json['properties'] as List<dynamic>)
          .map((e) => Property.fromJson(e as Map<String, dynamic>))
          .toList(),
    );

Map<String, dynamic> _$ConnectionToJson(Connection instance) =>
    <String, dynamic>{
      'targetMuid': instance.targetMuid,
      'deviceInfo': instance.deviceInfo,
      'isConnected': instance.isConnected,
      'profiles': instance.profiles,
      'properties': instance.properties,
    };

Profile _$ProfileFromJson(Map<String, dynamic> json) => Profile(
      profileId: json['profileId'] as String,
      group: (json['group'] as num).toInt(),
      address: (json['address'] as num).toInt(),
      enabled: json['enabled'] as bool,
      numChannelsRequested: (json['numChannelsRequested'] as num).toInt(),
    );

Map<String, dynamic> _$ProfileToJson(Profile instance) => <String, dynamic>{
      'profileId': instance.profileId,
      'group': instance.group,
      'address': instance.address,
      'enabled': instance.enabled,
      'numChannelsRequested': instance.numChannelsRequested,
    };

Property _$PropertyFromJson(Map<String, dynamic> json) => Property(
      propertyId: json['propertyId'] as String,
      resourceId: json['resourceId'] as String,
      name: json['name'] as String,
      mediaType: json['mediaType'] as String,
      data: (json['data'] as List<dynamic>)
          .map((e) => (e as num).toInt())
          .toList(),
    );

Map<String, dynamic> _$PropertyToJson(Property instance) => <String, dynamic>{
      'propertyId': instance.propertyId,
      'resourceId': instance.resourceId,
      'name': instance.name,
      'mediaType': instance.mediaType,
      'data': instance.data,
    };

MidiDevice _$MidiDeviceFromJson(Map<String, dynamic> json) => MidiDevice(
      deviceId: json['deviceId'] as String,
      name: json['name'] as String,
      isInput: json['isInput'] as bool,
    );

Map<String, dynamic> _$MidiDeviceToJson(MidiDevice instance) =>
    <String, dynamic>{
      'deviceId': instance.deviceId,
      'name': instance.name,
      'isInput': instance.isInput,
    };

LogEntry _$LogEntryFromJson(Map<String, dynamic> json) => LogEntry(
      timestamp: json['timestamp'] as String,
      isOutgoing: json['isOutgoing'] as bool,
      message: json['message'] as String,
    );

Map<String, dynamic> _$LogEntryToJson(LogEntry instance) => <String, dynamic>{
      'timestamp': instance.timestamp,
      'isOutgoing': instance.isOutgoing,
      'message': instance.message,
    };
