#include "midicci/details/commonproperties/CommonRulesPropertyHelper.hpp"
#include "midicci/details/PropertyCommonRules.hpp"
#include "midicci/details/MidiCIDevice.hpp"
#include "midicci/details/Json.hpp"
#include <sstream>

namespace midicci {
namespace commonproperties {

CommonRulesPropertyHelper::CommonRulesPropertyHelper(MidiCIDevice& device) 
    : device_(device) {}

std::vector<uint8_t> CommonRulesPropertyHelper::create_request_header_bytes(
    const std::string& property_id, const std::map<std::string, std::string>& fields) const {
    
    JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(property_id);
    
    for (const auto& [key, value] : fields) {
        if (!value.empty()) {
            if (key == PropertyCommonHeaderKeys::SET_PARTIAL) {
                header_obj[key] = JsonValue(value == "true");
            } else if (key == PropertyCommonHeaderKeys::OFFSET || key == PropertyCommonHeaderKeys::LIMIT) {
                header_obj[key] = JsonValue(std::stoi(value));
            } else {
                header_obj[key] = JsonValue(value);
            }
        }
    }
    
    JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyHelper::create_subscribe_header_bytes(
    const std::string& property_id, const std::string& command, const std::string& mutual_encoding) const {
    
    JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(property_id);
    header_obj[PropertyCommonHeaderKeys::COMMAND] = JsonValue(command);
    
    if (!mutual_encoding.empty()) {
        header_obj[PropertyCommonHeaderKeys::MUTUAL_ENCODING] = JsonValue(mutual_encoding);
    }
    
    JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_property_identifier_internal(const std::vector<uint8_t>& header) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(PropertyCommonHeaderKeys::RESOURCE);
            if (it != obj.end() && it->second.is_string()) {
                return it->second.as_string();
            }
        }
    } catch (...) {
    }
    return "";
}

std::vector<uint8_t> CommonRulesPropertyHelper::get_resource_list_request_bytes() const {
    JsonObject request_obj;
    request_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(PropertyResourceNames::RESOURCE_LIST);
    
    JsonValue request(request_obj);
    auto json_str = request.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.is_string()) {
                return it->second.as_string();
            }
        }
    } catch (...) {
    }
    return "";
}

int CommonRulesPropertyHelper::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.is_number()) {
                return it->second.as_int();
            }
        }
    } catch (...) {
    }
    return 0;
}

std::vector<uint8_t> CommonRulesPropertyHelper::encode_body(const std::vector<uint8_t>& data, const std::string& encoding) const {
    return data;
}

std::vector<uint8_t> CommonRulesPropertyHelper::decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) const {
    return body;
}

} // namespace properties
} // namespace midi_ci
