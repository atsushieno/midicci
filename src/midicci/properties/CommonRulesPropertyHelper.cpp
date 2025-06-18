#include "midicci/properties/CommonRulesPropertyHelper.hpp"
#include "midicci/properties/PropertyCommonRules.hpp"
#include "midicci/core/MidiCIDevice.hpp"
#include "midicci/json_ish/Json.hpp"
#include <sstream>

namespace midicci {
namespace properties {

using namespace property_common_rules;

CommonRulesPropertyHelper::CommonRulesPropertyHelper(core::MidiCIDevice& device) 
    : device_(device) {}

std::vector<uint8_t> CommonRulesPropertyHelper::create_request_header_bytes(
    const std::string& property_id, const std::map<std::string, std::string>& fields) const {
    
    json_ish::JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = json_ish::JsonValue(property_id);
    
    for (const auto& [key, value] : fields) {
        if (!value.empty()) {
            if (key == PropertyCommonHeaderKeys::SET_PARTIAL) {
                header_obj[key] = json_ish::JsonValue(value == "true");
            } else if (key == PropertyCommonHeaderKeys::OFFSET || key == PropertyCommonHeaderKeys::LIMIT) {
                header_obj[key] = json_ish::JsonValue(std::stoi(value));
            } else {
                header_obj[key] = json_ish::JsonValue(value);
            }
        }
    }
    
    json_ish::JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyHelper::create_subscribe_header_bytes(
    const std::string& property_id, const std::string& command, const std::string& mutual_encoding) const {
    
    json_ish::JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = json_ish::JsonValue(property_id);
    header_obj[PropertyCommonHeaderKeys::COMMAND] = json_ish::JsonValue(command);
    
    if (!mutual_encoding.empty()) {
        header_obj[PropertyCommonHeaderKeys::MUTUAL_ENCODING] = json_ish::JsonValue(mutual_encoding);
    }
    
    json_ish::JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_property_identifier_internal(const std::vector<uint8_t>& header) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = json_ish::JsonValue::parse(header_str);
        
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
    json_ish::JsonObject request_obj;
    request_obj[PropertyCommonHeaderKeys::RESOURCE] = json_ish::JsonValue(PropertyResourceNames::RESOURCE_LIST);
    
    json_ish::JsonValue request(request_obj);
    auto json_str = request.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = json_ish::JsonValue::parse(header_str);
        
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
        auto json_header = json_ish::JsonValue::parse(header_str);
        
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
