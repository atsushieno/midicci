#pragma once

#include "midicci/properties/ObservablePropertyList.hpp"
#include <string>
#include <vector>

namespace midicci {
namespace json_ish {
    class JsonValue;
}
namespace properties {

class CommonRulesPropertyMetadata : public PropertyMetadata {
public:
    enum class Originator {
        SYSTEM,
        USER
    };

    CommonRulesPropertyMetadata();
    explicit CommonRulesPropertyMetadata(const std::string& resource);
    
    std::string resource;
    bool canGet = true;
    std::string canSet = "none";
    bool canSubscribe = false;
    bool requireResId = false;
    std::vector<std::string> mediaTypes = {"application/json_ish"};
    std::vector<std::string> encodings = {"ASCII"};
    std::string schema;
    bool canPaginate = false;
    Originator originator = Originator::USER;
    std::vector<uint8_t> data;
    
    const std::string& getPropertyId() const override { return resource; }
    const std::string& getResourceId() const override { return resource; }
    const std::string& getName() const override { return resource; }
    const std::string& getMediaType() const override { return mediaTypes.empty() ? default_media_type : mediaTypes[0]; }
    const std::string& getEncoding() const override { return encodings.empty() ? default_encoding : encodings[0]; }
    const std::vector<uint8_t>& getData() const override { return data; }
    void setData(const std::vector<uint8_t>& newData) { data = newData; }
    
    std::string getExtra(const std::string& key) const override;
    
    midicci::json_ish::JsonValue toJsonValue() const;

private:
    static const std::string default_media_type;
    static const std::string default_encoding;
    static const std::vector<uint8_t> empty_data;
};

} // namespace properties
} // namespace midi_ci
