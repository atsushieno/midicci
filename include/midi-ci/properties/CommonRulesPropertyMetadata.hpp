#pragma once

#include "midi-ci/properties/MidiCIServicePropertyRules.hpp"
#include <string>
#include <vector>

namespace midi_ci {
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
    std::vector<std::string> mediaTypes = {"application/json"};
    std::vector<std::string> encodings = {"ASCII"};
    std::string schema;
    bool canPaginate = false;
    Originator originator = Originator::USER;
    
    std::string getPropertyId() const { return resource; }
    
    std::string getExtra(const std::string& key) const;
};

} // namespace properties
} // namespace midi_ci
