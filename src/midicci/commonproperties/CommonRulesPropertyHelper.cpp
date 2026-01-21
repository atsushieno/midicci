#include "midicci/midicci.hpp"
#include <sstream>

namespace midicci {
namespace commonproperties {

CommonRulesPropertyHelper::CommonRulesPropertyHelper(MidiCIDevice& device) 
    : device_(device) {}

std::vector<uint8_t> CommonRulesPropertyHelper::createRequestHeaderBytes(
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

std::vector<uint8_t> CommonRulesPropertyHelper::createSubscribeHeaderBytes(
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

std::string CommonRulesPropertyHelper::getPropertyIdentifierInternal(const std::vector<uint8_t>& header) const {
    std::string header_str(header.begin(), header.end());
    auto json_header = JsonValue::parse(header_str);

    if (json_header.isObject()) {
        const auto& obj = json_header.asObject();
        auto it = obj.find(PropertyCommonHeaderKeys::RESOURCE);
        if (it != obj.end() && it->second.isString()) {
            return it->second.asString();
        }
    }
    return "";
}

std::vector<uint8_t> CommonRulesPropertyHelper::getResourceListRequestBytes() const {
    JsonObject request_obj;
    request_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(PropertyResourceNames::RESOURCE_LIST);
    
    JsonValue request(request_obj);
    auto json_str = request.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::string CommonRulesPropertyHelper::getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.isObject()) {
            const auto& obj = json_header.asObject();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.isString()) {
                return it->second.asString();
            }
        }
    } catch (...) {
    }
    return "";
}

int CommonRulesPropertyHelper::getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.isObject()) {
            const auto& obj = json_header.asObject();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.isNumber()) {
                return it->second.asInt();
            }
        }
    } catch (...) {
    }
    return 0;
}

std::vector<uint8_t> CommonRulesPropertyHelper::encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) const {
    if (encoding == PropertyDataEncoding::ASCII || encoding.empty()) {
        return data;
    } else if (encoding == PropertyDataEncoding::MCODED7) {
        return PropertyCommonConverter::encodeToMcoded7(data);
    } else if (encoding == PropertyDataEncoding::ZLIB_MCODED7) {
        return PropertyCommonConverter::encodeToZlibMcoded7(data);
    } else {
        auto logger = device_.getLogger();
        if (logger) {
            logger(LogData("Unrecognized mutualEncoding is specified: " + encoding, true));
        }
        return data;
    }
}

std::vector<uint8_t> CommonRulesPropertyHelper::decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) const {
    auto encoding = getHeaderFieldString(header, PropertyCommonHeaderKeys::MUTUAL_ENCODING);
    return decodeBody(encoding.empty() ? std::nullopt : std::optional<std::string>(encoding), body);
}

std::vector<uint8_t> CommonRulesPropertyHelper::decodeBody(const std::optional<std::string>& encoding, const std::vector<uint8_t>& body) const {
    if (!encoding.has_value() || encoding.value() == PropertyDataEncoding::ASCII || encoding.value().empty()) {
        return body;
    } else if (encoding.value() == PropertyDataEncoding::MCODED7) {
        return PropertyCommonConverter::decodeMcoded7(body);
    } else if (encoding.value() == PropertyDataEncoding::ZLIB_MCODED7) {
        return PropertyCommonConverter::decodeZlibMcoded7(body);
    } else {
        auto logger = device_.getLogger();
        if (logger) {
            logger(LogData("Unrecognized mutualEncoding is specified: " + encoding.value(), true));
        }
        return body;
    }
}

} // namespace properties
} // namespace midi_ci
