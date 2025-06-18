#include "midi-ci/properties/ObservablePropertyList.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace midi_ci {
namespace properties {



SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                                   const std::string& sub_id, const std::string& enc)
    : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

} // namespace properties
} // namespace midi_ci
