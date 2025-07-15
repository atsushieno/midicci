#pragma once

#include "../MidiCIConstants.hpp"
#include "../MidiCIChannelList.hpp"
#include "../Json.hpp"
#include "../ObservablePropertyList.hpp"
#include <vector>
#include <cstdint>
#include <memory>

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

private:
    static JsonValue convertApplicationJsonBytesToJson(const std::vector<uint8_t>& data);
    static std::vector<std::unique_ptr<PropertyMetadata>> getMetadataListForBody(const JsonValue& body);
    static JsonValue bytesToJsonArray(const std::vector<uint8_t>& bytes);
    static JsonValue channelToJson(const MidiCIChannel& channel);
};

} // namespace commonproperties
} // namespace midicci