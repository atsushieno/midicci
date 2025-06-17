#include "midi-ci/properties/ObservablePropertyList.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace midi_ci {
namespace properties {

PropertyMetadata::PropertyMetadata(const std::string& id, const std::string& res_id, const std::string& prop_name,
                                 const std::string& type, const std::string& enc, const std::vector<uint8_t>& prop_data)
    : property_id(id), resource_id(res_id), name(prop_name), media_type(type), encoding(enc), data(prop_data) {}

SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                                   const std::string& sub_id, const std::string& enc)
    : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

} // namespace properties
} // namespace midi_ci
