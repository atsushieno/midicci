#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>

namespace midi_ci {
namespace properties {

class PropertyMetadata {
public:
    virtual ~PropertyMetadata() = default;
    
    virtual const std::string& getPropertyId() const = 0;
    virtual const std::string& getResourceId() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getMediaType() const = 0;
    virtual const std::string& getEncoding() const = 0;
    virtual const std::vector<uint8_t>& getData() const = 0;
    virtual std::string getExtra(const std::string& key) const = 0;
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
