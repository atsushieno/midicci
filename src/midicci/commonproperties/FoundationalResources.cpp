#include "midicci/commonproperties/FoundationalResources.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/PropertyCommonRules.hpp"
#include "midicci/MidiCIConverter.hpp"
#include <stdexcept>
#include <algorithm>
#include <optional>

namespace midicci::commonproperties {

JsonValue FoundationalResources::convertApplicationJsonBytesToJson(const std::vector<uint8_t>& data) {
    std::string json_str(data.begin(), data.end());
    return JsonValue::parse(json_str);
}

std::vector<std::unique_ptr<PropertyMetadata>> FoundationalResources::parseResourceList(const std::vector<uint8_t>& data) {
    const JsonValue json = convertApplicationJsonBytesToJson(data);
    return getMetadataListForBody(json);
}

std::vector<std::unique_ptr<PropertyMetadata>> FoundationalResources::getMetadataListForBody(const JsonValue& body) {
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    
    if (!body.is_array()) {
        throw std::runtime_error("Expected JSON array for resource list");
    }
    
    const auto& list = body.as_array();
    for (const auto& entry : list) {
        if (!entry.is_object()) {
            continue;
        }
        
        auto res = std::make_unique<CommonRulesPropertyMetadata>();
        const auto& entry_obj = entry.as_object();
        
        for (const auto& [key, value] : entry_obj) {
            if (key == PropertyResourceFields::RESOURCE && value.is_string()) {
                res->resource = value.as_string();
            } else if (key == PropertyResourceFields::CAN_GET && value.is_bool()) {
                res->canGet = value.as_bool();
            } else if (key == PropertyResourceFields::CAN_SET && value.is_string()) {
                res->canSet = value.as_string();
            } else if (key == PropertyResourceFields::CAN_SUBSCRIBE && value.is_bool()) {
                res->canSubscribe = value.as_bool();
            } else if (key == PropertyResourceFields::REQUIRE_RES_ID && value.is_bool()) {
                res->requireResId = value.as_bool();
            } else if (key == PropertyResourceFields::ENCODINGS && value.is_array()) {
                res->encodings.clear();
                for (const auto& e : value.as_array()) {
                    if (e.is_string()) {
                        res->encodings.push_back(e.as_string());
                    }
                }
            } else if (key == PropertyResourceFields::MEDIA_TYPE && value.is_array()) {
                res->mediaTypes.clear();
                for (const auto& e : value.as_array()) {
                    if (e.is_string()) {
                        res->mediaTypes.push_back(e.as_string());
                    }
                }
            } else if (key == PropertyResourceFields::SCHEMA && value.is_string()) {
                res->schema = value.as_string();
            } else if (key == PropertyResourceFields::CAN_PAGINATE && value.is_bool()) {
                res->canPaginate = value.as_bool();
            } else if (key == PropertyResourceFields::COLUMNS && value.is_array()) {
                res->columns.clear();
                for (const auto& c : value.as_array()) {
                    if (!c.is_object()) continue;
                    
                    PropertyResourceColumn col;
                    const auto& c_obj = c.as_object();
                    for (const auto& [col_key, col_value] : c_obj) {
                        if (col_key == PropertyResourceColumnFields::PROPERTY && col_value.is_string()) {
                            col.property = col_value.as_string();
                        } else if (col_key == PropertyResourceColumnFields::LINK && col_value.is_string()) {
                            col.link = col_value.as_string();
                        } else if (col_key == PropertyResourceColumnFields::TITLE && col_value.is_string()) {
                            col.title = col_value.as_string();
                        }
                    }
                    res->columns.push_back(col);
                }
            }
        }
        
        result.push_back(std::move(res));
    }
    
    return result;
}

DeviceInfo FoundationalResources::parseDeviceInfo(const std::vector<uint8_t>& data) {
    const JsonValue json = convertApplicationJsonBytesToJson(data);
    
    if (!json.is_object()) {
        throw std::runtime_error("Expected JSON object for device info");
    }
    
    const auto& obj = json.as_object();
    
    auto get_number = [&obj](const std::string& key, int default_value = 0) -> int {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.is_number()) {
            return static_cast<int>(it->second.as_number());
        }
        return default_value;
    };
    
    auto get_string = [&obj](const std::string& key, const std::string& default_value = "") -> std::string {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.is_string()) {
            return it->second.as_string();
        }
        return default_value;
    };
    
    return DeviceInfo(
        get_number(DeviceInfoPropertyNames::MANUFACTURER_ID),
        static_cast<uint16_t>(get_number(DeviceInfoPropertyNames::FAMILY_ID)),
        static_cast<uint16_t>(get_number(DeviceInfoPropertyNames::MODEL_ID)),
        get_number(DeviceInfoPropertyNames::VERSION_ID),
        get_string(DeviceInfoPropertyNames::MANUFACTURER),
        get_string(DeviceInfoPropertyNames::FAMILY),
        get_string(DeviceInfoPropertyNames::MODEL),
        get_string(DeviceInfoPropertyNames::VERSION),
        get_string(DeviceInfoPropertyNames::SERIAL_NUMBER)
    );
}

MidiCIChannelList FoundationalResources::parseChannelList(const std::vector<uint8_t>& data) {
    const JsonValue json = convertApplicationJsonBytesToJson(data);
    
    MidiCIChannelList channel_list;
    
    if (!json.is_array()) {
        return channel_list; // Return empty list if not array
    }
    
    const auto& array = json.as_array();
    for (const auto& item : array) {
        if (!item.is_object()) continue;
        
        const auto& obj = item.as_object();
        
        auto get_number = [&obj](const std::string& key, int default_value = 0) -> int {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.is_number()) {
                return static_cast<int>(it->second.as_number());
            }
            return default_value;
        };
        
        auto get_string = [&obj](const std::string& key, const std::string& default_value = "") -> std::string {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.is_string()) {
                return it->second.as_string();
            }
            return default_value;
        };
        
        // Parse bankPC array
        uint8_t bank_msb = 0, bank_lsb = 0, program = 0;
        auto bank_pc_it = obj.find(ChannelInfoPropertyNames::BANK_PC);
        if (bank_pc_it != obj.end() && bank_pc_it->second.is_array()) {
            const auto& bank_pc = bank_pc_it->second.as_array();
            if (bank_pc.size() >= 3) {
                if (bank_pc[0].is_number()) bank_msb = static_cast<uint8_t>(bank_pc[0].as_number());
                if (bank_pc[1].is_number()) bank_lsb = static_cast<uint8_t>(bank_pc[1].as_number());
                if (bank_pc[2].is_number()) program = static_cast<uint8_t>(bank_pc[2].as_number());
            }
        }
        
        // Parse MIDI mode
        int midi_mode = get_number(ChannelInfoPropertyNames::CLUSTER_MIDI_MODE, 3);
        bool is_omni_on = ((midi_mode - 1) & 1) != 0;
        bool is_poly_mode = ((midi_mode - 1) & 2) != 0;
        
        MidiCIChannel channel(
            get_string(ChannelInfoPropertyNames::TITLE),
            get_number(ChannelInfoPropertyNames::CHANNEL, 1) - 1, // Convert from 1-based to 0-based
            get_string(ChannelInfoPropertyNames::PROGRAM_TITLE),
            bank_msb,
            bank_lsb,
            program,
            get_number(ChannelInfoPropertyNames::CLUSTER_CHANNEL_START, 1) - 1, // Convert from 1-based to 0-based
            get_number(ChannelInfoPropertyNames::CLUSTER_LENGTH, 1),
            is_omni_on,
            is_poly_mode,
            get_string(ChannelInfoPropertyNames::CLUSTER_TYPE)
        );
        
        channel_list.channels.push_back(channel);
    }
    
    return channel_list;
}

JsonValue FoundationalResources::parseJsonSchema(const std::vector<uint8_t>& data) {
    return convertApplicationJsonBytesToJson(data);
}

JsonValue FoundationalResources::toJsonValue(const std::vector<std::unique_ptr<PropertyMetadata>>& resourceList) {
    JsonArray array;
    
    for (const auto& metadata : resourceList) {
        const auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
        if (common_metadata) {
            array.push_back(common_metadata->toJsonValue());
        }
    }
    
    return JsonValue(array);
}

JsonValue FoundationalResources::bytesToJsonArray(const std::vector<uint8_t>& bytes) {
    JsonArray array;
    for (uint8_t byte : bytes) {
        array.push_back(JsonValue(static_cast<double>(byte)));
    }
    return JsonValue(array);
}

JsonValue FoundationalResources::toJsonValue(const DeviceInfo& deviceInfo) {
    JsonObject obj;
    
    // Convert IDs to byte arrays following Kotlin pattern
    std::vector<uint8_t> manufacturer_bytes = {
        static_cast<uint8_t>((deviceInfo.manufacturer_id >> 16) & 0xFF),
        static_cast<uint8_t>((deviceInfo.manufacturer_id >> 8) & 0xFF),
        static_cast<uint8_t>(deviceInfo.manufacturer_id & 0xFF)
    };
    obj[DeviceInfoPropertyNames::MANUFACTURER_ID] = bytesToJsonArray(manufacturer_bytes);
    
    std::vector<uint8_t> family_bytes = {
        static_cast<uint8_t>((deviceInfo.family_id >> 8) & 0xFF),
        static_cast<uint8_t>(deviceInfo.family_id & 0xFF)
    };
    obj[DeviceInfoPropertyNames::FAMILY_ID] = bytesToJsonArray(family_bytes);
    
    std::vector<uint8_t> model_bytes = {
        static_cast<uint8_t>((deviceInfo.model_id >> 8) & 0xFF),
        static_cast<uint8_t>(deviceInfo.model_id & 0xFF)
    };
    obj[DeviceInfoPropertyNames::MODEL_ID] = bytesToJsonArray(model_bytes);
    
    std::vector<uint8_t> version_bytes = {
        static_cast<uint8_t>((deviceInfo.version_id >> 24) & 0xFF),
        static_cast<uint8_t>((deviceInfo.version_id >> 16) & 0xFF),
        static_cast<uint8_t>((deviceInfo.version_id >> 8) & 0xFF),
        static_cast<uint8_t>(deviceInfo.version_id & 0xFF)
    };
    obj[DeviceInfoPropertyNames::VERSION_ID] = bytesToJsonArray(version_bytes);
    
    obj[DeviceInfoPropertyNames::MANUFACTURER] = JsonValue(deviceInfo.manufacturer);
    obj[DeviceInfoPropertyNames::FAMILY] = JsonValue(deviceInfo.family);
    obj[DeviceInfoPropertyNames::MODEL] = JsonValue(deviceInfo.model);
    obj[DeviceInfoPropertyNames::VERSION] = JsonValue(deviceInfo.version);
    
    if (!deviceInfo.serial_number.empty()) {
        obj[DeviceInfoPropertyNames::SERIAL_NUMBER] = JsonValue(deviceInfo.serial_number);
    }
    
    return JsonValue(obj);
}

JsonValue FoundationalResources::toJsonValue(const MidiCIChannelList& channelList) {
    if (channelList.channels.empty()) {
        return JsonValue(); // null
    }
    
    JsonArray array;
    for (const auto& channel : channelList.channels) {
        array.push_back(channelToJson(channel));
    }
    
    return JsonValue(array);
}

JsonValue FoundationalResources::channelToJson(const MidiCIChannel& channel) {
    JsonObject obj;
    
    obj[ChannelInfoPropertyNames::TITLE] = JsonValue(channel.title);
    obj[ChannelInfoPropertyNames::CHANNEL] = JsonValue(static_cast<double>(channel.channel + 1)); // Convert to 1-based
    
    if (!channel.program_title.empty()) {
        obj[ChannelInfoPropertyNames::PROGRAM_TITLE] = JsonValue(channel.program_title);
    }
    
    // Include bankPC only if any values are non-zero
    if (channel.bank_msb != 0 || channel.bank_lsb != 0 || channel.program != 0) {
        JsonArray bank_pc;
        bank_pc.push_back(JsonValue(static_cast<double>(channel.bank_msb)));
        bank_pc.push_back(JsonValue(static_cast<double>(channel.bank_lsb)));
        bank_pc.push_back(JsonValue(static_cast<double>(channel.program)));
        obj[ChannelInfoPropertyNames::BANK_PC] = JsonValue(bank_pc);
    }
    
    if (channel.cluster_channel_start > 0) {
        obj[ChannelInfoPropertyNames::CLUSTER_CHANNEL_START] = JsonValue(static_cast<double>(channel.cluster_channel_start + 1)); // Convert to 1-based
    }
    
    if (channel.cluster_length > 1) {
        obj[ChannelInfoPropertyNames::CLUSTER_LENGTH] = JsonValue(static_cast<double>(channel.cluster_length));
    }
    
    // Calculate MIDI mode (default is 3 for poly mode)
    uint8_t midi_mode = 3; // Default poly mode
    if (!channel.is_poly_mode) {
        midi_mode = channel.is_omni_on ? 1 : 2;
    }
    if (midi_mode != 3) {
        obj[ChannelInfoPropertyNames::CLUSTER_MIDI_MODE] = JsonValue(static_cast<double>(midi_mode));
    }
    
    if (!channel.cluster_type.empty() && channel.cluster_type != "OTHER") {
        obj[ChannelInfoPropertyNames::CLUSTER_TYPE] = JsonValue(channel.cluster_type);
    }
    
    return JsonValue(obj);
}

// Extension methods for ObservablePropertyList (matching Kotlin extension properties)
std::vector<std::unique_ptr<PropertyMetadata>> FoundationalResources::getResourceList(const midicci::ObservablePropertyList& propertyList) {
    return propertyList.getMetadataList();
}

std::optional<DeviceInfo> FoundationalResources::getDeviceInfo(const midicci::ObservablePropertyList& propertyList) {
    auto values = propertyList.getValues();
    auto it = std::find_if(values.begin(), values.end(), 
        [](const PropertyValue& pv) { return pv.id == PropertyResourceNames::DEVICE_INFO; });
    
    if (it != values.end()) {
        try {
            return FoundationalResources::parseDeviceInfo(it->body);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<MidiCIChannelList> FoundationalResources::getChannelList(const midicci::ObservablePropertyList& propertyList) {
    auto values = propertyList.getValues();
    auto it = std::find_if(values.begin(), values.end(), 
        [](const PropertyValue& pv) { return pv.id == PropertyResourceNames::CHANNEL_LIST; });
    
    if (it != values.end()) {
        try {
            return FoundationalResources::parseChannelList(it->body);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<JsonValue> FoundationalResources::getJsonSchema(const midicci::ObservablePropertyList& propertyList) {
    auto values = propertyList.getValues();
    auto it = std::find_if(values.begin(), values.end(), 
        [](const PropertyValue& pv) { return pv.id == PropertyResourceNames::JSON_SCHEMA; });
    
    if (it != values.end()) {
        try {
            return FoundationalResources::parseJsonSchema(it->body);
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

} // namespace midicci::commonproperties