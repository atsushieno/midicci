#include "midicci/midicci.hpp"
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
    
    if (!body.isArray()) {
        throw std::runtime_error("Expected JSON array for resource list");
    }
    
    const auto& list = body.asArray();
    for (const auto& entry : list) {
        if (!entry.isObject()) {
            continue;
        }
        
        auto res = std::make_unique<CommonRulesPropertyMetadata>();
        const auto& entry_obj = entry.asObject();
        
        for (const auto& [key, value] : entry_obj) {
            if (key == PropertyResourceFields::RESOURCE && value.isString()) {
                res->resource = value.asString();
            } else if (key == PropertyResourceFields::CAN_GET && value.isBool()) {
                res->canGet = value.asBool();
            } else if (key == PropertyResourceFields::CAN_SET && value.isString()) {
                res->canSet = value.asString();
            } else if (key == PropertyResourceFields::CAN_SUBSCRIBE && value.isBool()) {
                res->canSubscribe = value.asBool();
            } else if (key == PropertyResourceFields::REQUIRE_RES_ID && value.isBool()) {
                res->requireResId = value.asBool();
            } else if (key == PropertyResourceFields::ENCODINGS && value.isArray()) {
                res->encodings.clear();
                for (const auto& e : value.asArray()) {
                    if (e.isString()) {
                        res->encodings.push_back(e.asString());
                    }
                }
            } else if (key == PropertyResourceFields::MEDIA_TYPE && value.isArray()) {
                res->mediaTypes.clear();
                for (const auto& e : value.asArray()) {
                    if (e.isString()) {
                        res->mediaTypes.push_back(e.asString());
                    }
                }
            } else if (key == PropertyResourceFields::SCHEMA && value.isString()) {
                res->schema = value.asString();
            } else if (key == PropertyResourceFields::CAN_PAGINATE && value.isBool()) {
                res->canPaginate = value.asBool();
            } else if (key == PropertyResourceFields::COLUMNS && value.isArray()) {
                res->columns.clear();
                for (const auto& c : value.asArray()) {
                    if (!c.isObject()) continue;
                    
                    PropertyResourceColumn col;
                    const auto& c_obj = c.asObject();
                    for (const auto& [col_key, col_value] : c_obj) {
                        if (col_key == PropertyResourceColumnFields::PROPERTY && col_value.isString()) {
                            col.property = col_value.asString();
                        } else if (col_key == PropertyResourceColumnFields::LINK && col_value.isString()) {
                            col.link = col_value.asString();
                        } else if (col_key == PropertyResourceColumnFields::TITLE && col_value.isString()) {
                            col.title = col_value.asString();
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
    
    if (!json.isObject()) {
        throw std::runtime_error("Expected JSON object for device info");
    }
    
    const auto& obj = json.asObject();
    
    auto get_number = [&obj](const std::string& key, int default_value = 0) -> int {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.isNumber()) {
            return static_cast<int>(it->second.asNumber());
        }
        return default_value;
    };
    
    auto getString = [&obj](const std::string& key, const std::string& default_value = "") -> std::string {
        auto it = obj.find(key);
        if (it != obj.end() && it->second.isString()) {
            return it->second.asString();
        }
        return default_value;
    };
    
    return DeviceInfo(
        get_number(DeviceInfoPropertyNames::MANUFACTURER_ID),
        static_cast<uint16_t>(get_number(DeviceInfoPropertyNames::FAMILY_ID)),
        static_cast<uint16_t>(get_number(DeviceInfoPropertyNames::MODEL_ID)),
        get_number(DeviceInfoPropertyNames::VERSION_ID),
        getString(DeviceInfoPropertyNames::MANUFACTURER),
        getString(DeviceInfoPropertyNames::FAMILY),
        getString(DeviceInfoPropertyNames::MODEL),
        getString(DeviceInfoPropertyNames::VERSION),
        getString(DeviceInfoPropertyNames::SERIAL_NUMBER)
    );
}

MidiCIChannelList FoundationalResources::parseChannelList(const std::vector<uint8_t>& data) {
    const JsonValue json = convertApplicationJsonBytesToJson(data);
    
    MidiCIChannelList channel_list;
    
    if (!json.isArray()) {
        return channel_list; // Return empty list if not array
    }
    
    const auto& array = json.asArray();
    for (const auto& item : array) {
        if (!item.isObject()) continue;
        
        const auto& obj = item.asObject();
        
        auto get_number = [&obj](const std::string& key, int default_value = 0) -> int {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.isNumber()) {
                return static_cast<int>(it->second.asNumber());
            }
            return default_value;
        };
        
        auto getString = [&obj](const std::string& key, const std::string& default_value = "") -> std::string {
            auto it = obj.find(key);
            if (it != obj.end() && it->second.isString()) {
                return it->second.asString();
            }
            return default_value;
        };
        
        // Parse bankPC array
        uint8_t bank_msb = 0, bank_lsb = 0, program = 0;
        auto bank_pc_it = obj.find(ChannelInfoPropertyNames::BANK_PC);
        if (bank_pc_it != obj.end() && bank_pc_it->second.isArray()) {
            const auto& bank_pc = bank_pc_it->second.asArray();
            if (bank_pc.size() >= 3) {
                if (bank_pc[0].isNumber()) bank_msb = static_cast<uint8_t>(bank_pc[0].asNumber());
                if (bank_pc[1].isNumber()) bank_lsb = static_cast<uint8_t>(bank_pc[1].asNumber());
                if (bank_pc[2].isNumber()) program = static_cast<uint8_t>(bank_pc[2].asNumber());
            }
        }
        
        // Parse MIDI mode
        int midi_mode = get_number(ChannelInfoPropertyNames::CLUSTER_MIDI_MODE, 3);
        bool is_omni_on = ((midi_mode - 1) & 1) != 0;
        bool is_poly_mode = ((midi_mode - 1) & 2) != 0;

        MidiCIChannel channel(
            getString(ChannelInfoPropertyNames::TITLE),
            get_number(ChannelInfoPropertyNames::CHANNEL, 1) - 1, // Convert from 1-based to 0-based
            getString(ChannelInfoPropertyNames::PROGRAM_TITLE),
            bank_msb,
            bank_lsb,
            program,
            get_number(ChannelInfoPropertyNames::CLUSTER_CHANNEL_START, 1) - 1, // Convert from 1-based to 0-based
            get_number(ChannelInfoPropertyNames::CLUSTER_LENGTH, 1),
            is_omni_on,
            is_poly_mode,
            getString(ChannelInfoPropertyNames::CLUSTER_TYPE)
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