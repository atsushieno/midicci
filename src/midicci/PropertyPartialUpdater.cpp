#include "midicci/details/PropertyPartialUpdater.hpp"
#include <algorithm>

namespace midicci {

static std::string replace_all(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
    return str;
}

std::vector<std::string> PropertyPartialUpdater::parseJsonPointer(const std::string& s) {
    if (s.empty() || s[0] != '/')
        return {};

    std::vector<std::string> result;
    std::string remaining = s.substr(1);
    size_t pos = 0;

    while ((pos = remaining.find('/')) != std::string::npos) {
        std::string token = remaining.substr(0, pos);
        token = replace_all(token, "~1", "/");
        token = replace_all(token, "~0", "~");
        result.push_back(token);
        remaining = remaining.substr(pos + 1);
    }

    if (!remaining.empty()) {
        remaining = replace_all(remaining, "~1", "/");
        remaining = replace_all(remaining, "~0", "~");
        result.push_back(remaining);
    }

    return result;
}

JsonValue PropertyPartialUpdater::applyPartialUpdate(const JsonValue& obj, const std::string& path, const JsonValue& value) {
    return applyPartialUpdate(obj, parseJsonPointer(path), value);
}

JsonValue PropertyPartialUpdater::applyPartialUpdate(const JsonValue& obj, const std::vector<std::string>& json_pointer_path, const JsonValue& value) {
    return patch(obj, json_pointer_path, value);
}

JsonValue PropertyPartialUpdater::patch(const JsonValue& obj, const std::vector<std::string>& path, const JsonValue& value) {
    if (path.empty())
        return obj;

    if (!obj.isObject())
        return obj;

    const auto& obj_map = obj.asObject();
    const std::string& entry = path[0];

    auto context_key_it = obj_map.find(entry);

    if (context_key_it == obj_map.end())
        return obj;

    JsonValue replacement;
    if (path.size() == 1) {
        replacement = value;
    } else {
        std::vector<std::string> remaining_path(path.begin() + 1, path.end());
        replacement = patch(context_key_it->second, remaining_path, value);
    }

    JsonObject new_map;
    for (const auto& [k, v] : obj_map) {
        if (k == context_key_it->first) {
            new_map[k] = replacement;
        } else {
            new_map[k] = v;
        }
    }

    return JsonValue(new_map);
}

std::pair<bool, JsonValue> PropertyPartialUpdater::applyPartialUpdates(const JsonValue& existing_json, const JsonValue& partial_spec_json) {
    if (!partial_spec_json.isObject())
        return {false, existing_json};

    JsonValue target = existing_json;
    const auto& spec_obj = partial_spec_json.asObject();

    for (const auto& [path, new_value] : spec_obj) {
        target = applyPartialUpdate(target, path, new_value);
    }

    return {true, target};
}

}
