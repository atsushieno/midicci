#include "midicci/properties/CommonRulesPropertyMetadata.hpp"
#include "midicci/properties/PropertyCommonRules.hpp"
#include "midicci/json_ish/Json.hpp"
#include <algorithm>

namespace midicci {
namespace properties {

const std::string CommonRulesPropertyMetadata::default_media_type = "application/json";
const std::string CommonRulesPropertyMetadata::default_encoding = "ASCII";
const std::vector<uint8_t> CommonRulesPropertyMetadata::empty_data = {};

CommonRulesPropertyMetadata::CommonRulesPropertyMetadata() {
}

CommonRulesPropertyMetadata::CommonRulesPropertyMetadata(const std::string& resource) 
    : resource(resource) {
}

std::string CommonRulesPropertyMetadata::getExtra(const std::string& key) const {
    if (key == "mediaTypes") {
        std::string result = "[";
        for (size_t i = 0; i < mediaTypes.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + mediaTypes[i] + "\"";
        }
        result += "]";
        return result;
    } else if (key == "encodings") {
        std::string result = "[";
        for (size_t i = 0; i < encodings.size(); ++i) {
            if (i > 0) result += ",";
            result += "\"" + encodings[i] + "\"";
        }
        result += "]";
        return result;
    }
    return "";
}

midicci::json_ish::JsonValue CommonRulesPropertyMetadata::toJsonValue() const {
    using namespace midicci::json_ish;
    using namespace midicci::properties::property_common_rules;
    
    JsonObject obj;
    
    obj[PropertyResourceFields::RESOURCE] = JsonValue(resource);
    
    if (!canGet) {
        obj[PropertyResourceFields::CAN_GET] = JsonValue(canGet);
    }
    
    if (canSet != PropertySetAccess::NONE) {
        obj[PropertyResourceFields::CAN_SET] = JsonValue(canSet);
    }
    
    if (canSubscribe) {
        obj[PropertyResourceFields::CAN_SUBSCRIBE] = JsonValue(canSubscribe);
    }
    
    if (requireResId) {
        obj[PropertyResourceFields::REQUIRE_RES_ID] = JsonValue(requireResId);
    }
    
    if (mediaTypes.size() != 1 || mediaTypes[0] != "application/json") {
        JsonArray mediaTypesArray;
        for (const auto& mediaType : mediaTypes) {
            mediaTypesArray.push_back(JsonValue(mediaType));
        }
        obj[PropertyResourceFields::MEDIA_TYPE] = JsonValue(mediaTypesArray);
    }
    
    if (encodings.size() != 1 || encodings[0] != "ASCII") {
        JsonArray encodingsArray;
        for (const auto& encoding : encodings) {
            encodingsArray.push_back(JsonValue(encoding));
        }
        obj[PropertyResourceFields::ENCODINGS] = JsonValue(encodingsArray);
    }
    
    if (!schema.empty()) {
        try {
            obj[PropertyResourceFields::SCHEMA] = JsonValue::parse(schema);
        } catch (...) {
            obj[PropertyResourceFields::SCHEMA] = JsonValue(schema);
        }
    }
    
    if (canPaginate) {
        obj[PropertyResourceFields::CAN_PAGINATE] = JsonValue(canPaginate);
    }
    
    return JsonValue(obj);
}

} // namespace properties
} // namespace midi_ci
