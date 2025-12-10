#pragma once

#include "midicci/midicci.hpp"
#include <string>
#include <vector>

namespace midicci::commonproperties {
    struct PropertyResourceColumn {
        std::string property;
        std::string link;
        std::string title;
    };
}

namespace midicci::commonproperties {

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
    std::vector<std::string> mediaTypes = {"application/json"};
    std::vector<std::string> encodings = {"ASCII"};
    std::string schema;
    bool canPaginate = false;
    std::vector<PropertyResourceColumn> columns;
    Originator originator = Originator::USER;

    const std::string& getPropertyId() const override { return resource; }
    std::string getExtra(const std::string& key) const override;
    
    midicci::JsonValue toJsonValue() const;

private:
    const std::string default_media_type = "application/json";
    const std::string default_encoding = "ASCII";
    const std::vector<uint8_t> empty_data = {};
};

} // namespace
