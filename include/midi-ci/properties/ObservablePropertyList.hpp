#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>

namespace midi_ci {
namespace properties {

struct PropertyMetadata {
    std::string property_id;
    std::string resource_id;
    std::string name;
    std::string media_type;
    std::string encoding;
    std::vector<uint8_t> data;
    
    PropertyMetadata(const std::string& id, const std::string& res_id, const std::string& prop_name,
                    const std::string& type, const std::string& enc, const std::vector<uint8_t>& prop_data);
};

struct SubscriptionEntry {
    uint32_t muid;
    std::string resource;
    std::string subscribe_id;
    std::string encoding;
    
    SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                     const std::string& sub_id, const std::string& enc);
};

} // namespace properties
} // namespace midi_ci
