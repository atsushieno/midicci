#pragma once

#include "midicci/details/Json.hpp"
#include <string>
#include <vector>
#include <utility>

namespace midicci {

class PropertyPartialUpdater {
public:
    static std::vector<std::string> parseJsonPointer(const std::string& s);

    static JsonValue applyPartialUpdate(const JsonValue& obj, const std::string& path, const JsonValue& value);

    static JsonValue applyPartialUpdate(const JsonValue& obj, const std::vector<std::string>& json_pointer_path, const JsonValue& value);

    static std::pair<bool, JsonValue> applyPartialUpdates(const JsonValue& existing_json, const JsonValue& partial_spec_json);

private:
    static JsonValue patch(const JsonValue& obj, const std::vector<std::string>& path, const JsonValue& value);
};

}
