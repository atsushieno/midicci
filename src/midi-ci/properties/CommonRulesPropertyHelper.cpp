#include "midi-ci/properties/CommonRulesPropertyHelper.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/json/Json.hpp"
#include <sstream>

namespace midi_ci {
namespace properties {

using namespace property_common_rules;

CommonRulesPropertyHelper::CommonRulesPropertyHelper(core::MidiCIDevice& device) 
    : device_(device) {}

std::vector<uint8_t> CommonRulesPropertyHelper::create_request_header_bytes(
    const std::string& property_id, const std::map<std::string, std::string>& fields) const {
    
    json::JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = json::JsonValue(property_id);
    
    for (const auto& [key, value] : fields) {
        if (!value.empty()) {
            if (key == PropertyCommonHeaderKeys::SET_PARTIAL) {
                header_obj[key] = json::JsonValue(value == "true");
            } else if (key == PropertyCommonHeaderKeys::OFFSET || key == PropertyCommonHeaderKeys::LIMIT) {
                header_obj[key] = json::JsonValue(std::stoi(value));
            } else {
                header_obj[key] = json::JsonValue(value);
            }
        }
    }
    
    json::JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyHelper::create_subscribe_header_bytes(
    const std::string& property_id, const std::string& command, const std::string& mutual_encoding) const {
    
    json::JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = json::JsonValue(property_id);
    header_obj[PropertyCommonHeaderKeys::COMMAND] = json::JsonValue(command);
    
    if (!mutual_encoding.empty()) {
        header_obj[PropertyCommonHeaderKeys::MUTUAL_ENCODING] = json::JsonValue(mutual_encoding);
    }
    
    json::JsonValue header(header_obj);
    auto json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_property_identifier_internal(const std::vector<uint8_t>& header) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = json::JsonValue::parse(header_str);
        
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
    json::JsonObject request_obj;
    request_obj[PropertyCommonHeaderKeys::RESOURCE] = json::JsonValue(PropertyResourceNames::RESOURCE_LIST);
    
    json::JsonValue request(request_obj);
    auto json_str = request.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = json::JsonValue::parse(header_str);
        
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
        auto json_header = json::JsonValue::parse(header_str);
        
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
