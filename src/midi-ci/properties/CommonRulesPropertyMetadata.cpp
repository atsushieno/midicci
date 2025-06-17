#include "midi-ci/properties/CommonRulesPropertyMetadata.hpp"
#include <algorithm>

namespace midi_ci {
namespace properties {

CommonRulesPropertyMetadata::CommonRulesPropertyMetadata() 
    : PropertyMetadata("", "", "", "application/json", {}) {
}

CommonRulesPropertyMetadata::CommonRulesPropertyMetadata(const std::string& resource) 
    : PropertyMetadata(resource, resource, "", "application/json", {}), resource(resource) {
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
