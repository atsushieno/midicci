#include "midicci/properties/CommonRulesPropertyMetadata.hpp"
#include <algorithm>

namespace midicci {
namespace properties {

const std::string CommonRulesPropertyMetadata::default_media_type = "application/json_ish";
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

} // namespace properties
} // namespace midi_ci
