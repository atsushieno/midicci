#pragma once

#include "midicci/details/Json.hpp"
#include <string>
#include <vector>
#include <utility>

namespace midicci {

class PropertyPartialUpdater {
public:
    static std::vector<std::string> parse_json_pointer(const std::string& s);

    static JsonValue apply_partial_update(const JsonValue& obj, const std::string& path, const JsonValue& value);

    static JsonValue apply_partial_update(const JsonValue& obj, const std::vector<std::string>& json_pointer_path, const JsonValue& value);

    static std::pair<bool, JsonValue> apply_partial_updates(const JsonValue& existing_json, const JsonValue& partial_spec_json);

private:
    static JsonValue patch(const JsonValue& obj, const std::vector<std::string>& path, const JsonValue& value);
};

}
