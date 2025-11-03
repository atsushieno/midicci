#pragma once

#include "midicci/midicci.hpp"
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>

namespace midicci {
namespace commonproperties {

struct PropertyResourceColumnFields {
    static constexpr const char* PROPERTY = "property";
    static constexpr const char* LINK = "link";
    static constexpr const char* TITLE = "title";
};

class FoundationalResources {
public:
    // JSON bytes to strongly-typed info
    static std::vector<std::unique_ptr<PropertyMetadata>> parseResourceList(const std::vector<uint8_t>& data);
    static DeviceInfo parseDeviceInfo(const std::vector<uint8_t>& data);
    static MidiCIChannelList parseChannelList(const std::vector<uint8_t>& data);
    static JsonValue parseJsonSchema(const std::vector<uint8_t>& data);
    
    // Strongly-typed objects to JSON
    static JsonValue toJsonValue(const std::vector<std::unique_ptr<PropertyMetadata>>& resourceList);
    static JsonValue toJsonValue(const DeviceInfo& deviceInfo);
    static JsonValue toJsonValue(const MidiCIChannelList& channelList);
    
    // Extension methods for ObservablePropertyList (matching Kotlin extension properties)
    static std::vector<std::unique_ptr<PropertyMetadata>> getResourceList(const midicci::ObservablePropertyList& propertyList);
    static std::optional<DeviceInfo> getDeviceInfo(const midicci::ObservablePropertyList& propertyList);
    static std::optional<MidiCIChannelList> getChannelList(const midicci::ObservablePropertyList& propertyList);
    static std::optional<JsonValue> getJsonSchema(const midicci::ObservablePropertyList& propertyList);

private:
    static JsonValue convertApplicationJsonBytesToJson(const std::vector<uint8_t>& data);
    static std::vector<std::unique_ptr<PropertyMetadata>> getMetadataListForBody(const JsonValue& body);
    static JsonValue bytesToJsonArray(const std::vector<uint8_t>& bytes);
    static JsonValue channelToJson(const MidiCIChannel& channel);
};

} // namespace commonproperties
} // namespace midicci