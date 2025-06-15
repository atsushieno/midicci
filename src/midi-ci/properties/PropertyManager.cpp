#include "midi-ci/properties/PropertyManager.hpp"
#include <mutex>
#include <vector>
#include <algorithm>
#include <sstream>

namespace midi_ci {
namespace properties {

PropertyMetadata::PropertyMetadata(const std::string& id, const std::string& res_id, const std::string& prop_name,
                                 const std::string& type, const std::string& enc, const std::vector<uint8_t>& prop_data)
    : property_id(id), resource_id(res_id), name(prop_name), media_type(type), encoding(enc), data(prop_data) {}

SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                                   const std::string& sub_id, const std::string& enc)
    : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

class PropertyManager::Impl {
public:
    std::vector<PropertyMetadata> properties_;
    std::vector<SubscriptionEntry> subscriptions_;
    std::vector<PropertyUpdateCallback> update_callbacks_;
    std::vector<SubscriptionCallback> subscription_callbacks_;
    uint8_t max_simultaneous_requests_;
    mutable std::mutex mutex_;
    
    Impl() : max_simultaneous_requests_(1) {}
};

PropertyManager::PropertyManager() : pimpl_(std::make_unique<Impl>()) {}

PropertyManager::~PropertyManager() = default;

void PropertyManager::add_property(const PropertyMetadata& property) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property.property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        *it = property;
    } else {
        pimpl_->properties_.push_back(property);
    }
}

void PropertyManager::remove_property(const std::string& property_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        pimpl_->properties_.erase(it);
    }
    
    auto sub_it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const SubscriptionEntry& s) {
            return s.subscribe_id == property_id;
        });
    pimpl_->subscriptions_.erase(sub_it, pimpl_->subscriptions_.end());
}

void PropertyManager::update_property_metadata(const std::string& old_property_id, const PropertyMetadata& property) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == old_property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        *it = property;
    }
}

void PropertyManager::set_property_value(const std::string& property_id, const std::string& resource_id,
                                        const std::vector<uint8_t>& data, bool is_partial) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property_id && p.resource_id == resource_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        if (is_partial) {
            it->data.insert(it->data.end(), data.begin(), data.end());
        } else {
            it->data = data;
        }
        
        for (const auto& callback : pimpl_->update_callbacks_) {
            callback(property_id, it->data, is_partial);
        }
        
        notify_property_update(property_id, it->data, is_partial);
    }
}

std::vector<PropertyMetadata> PropertyManager::get_metadata_list() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->properties_;
}

PropertyMetadata* PropertyManager::find_property(const std::string& property_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property_id;
        });
    
    return (it != pimpl_->properties_.end()) ? &(*it) : nullptr;
}

const PropertyMetadata* PropertyManager::find_property(const std::string& property_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property_id;
        });
    
    return (it != pimpl_->properties_.end()) ? &(*it) : nullptr;
}

std::vector<uint8_t> PropertyManager::get_property_value(const std::string& property_id, const std::string& resource_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&](const PropertyMetadata& p) {
            return p.property_id == property_id && p.resource_id == resource_id;
        });
    
    return (it != pimpl_->properties_.end()) ? it->data : std::vector<uint8_t>();
}

void PropertyManager::add_subscription(const SubscriptionEntry& subscription) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const SubscriptionEntry& s) {
            return s.muid == subscription.muid && s.subscribe_id == subscription.subscribe_id;
        });
    
    if (it == pimpl_->subscriptions_.end()) {
        pimpl_->subscriptions_.push_back(subscription);
        
        for (const auto& callback : pimpl_->subscription_callbacks_) {
            callback(subscription);
        }
    }
}

void PropertyManager::remove_subscription(uint32_t muid, const std::string& property_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const SubscriptionEntry& s) {
            return s.muid == muid && s.subscribe_id == property_id;
        });
    
    if (it != pimpl_->subscriptions_.end()) {
        pimpl_->subscriptions_.erase(it);
    }
}

void PropertyManager::remove_all_subscriptions(uint32_t muid) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const SubscriptionEntry& s) {
            return s.muid == muid;
        });
    
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
}

std::vector<SubscriptionEntry> PropertyManager::get_subscriptions() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

std::vector<SubscriptionEntry> PropertyManager::get_subscriptions_for_property(const std::string& property_id) const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    std::vector<SubscriptionEntry> property_subscriptions;
    std::copy_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
                 std::back_inserter(property_subscriptions),
                 [&](const SubscriptionEntry& s) { return s.subscribe_id == property_id; });
    
    return property_subscriptions;
}

void PropertyManager::notify_property_update(const std::string& property_id, const std::vector<uint8_t>& data, bool is_partial) {
    auto subscriptions = get_subscriptions_for_property(property_id);
    
    for (const auto& subscription : subscriptions) {
        for (const auto& callback : pimpl_->subscription_callbacks_) {
            callback(subscription);
        }
    }
}

void PropertyManager::set_property_update_callback(PropertyUpdateCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->update_callbacks_.clear();
    pimpl_->update_callbacks_.push_back(callback);
}

void PropertyManager::add_subscription_callback(SubscriptionCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->subscription_callbacks_.push_back(callback);
}

void PropertyManager::remove_subscription_callback(SubscriptionCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->subscription_callbacks_.begin(), pimpl_->subscription_callbacks_.end(),
        [&callback](const SubscriptionCallback& cb) {
            return cb.target_type() == callback.target_type();
        });
    if (it != pimpl_->subscription_callbacks_.end()) {
        pimpl_->subscription_callbacks_.erase(it);
    }
}

uint8_t PropertyManager::get_max_simultaneous_requests() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->max_simultaneous_requests_;
}

void PropertyManager::set_max_simultaneous_requests(uint8_t max_requests) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->max_simultaneous_requests_ = max_requests;
}

std::vector<uint8_t> PropertyManager::create_property_header(const std::string& property_id, 
                                                           const std::map<std::string, std::string>& headers) const {
    std::ostringstream oss;
    oss << "X-Property-Id: " << property_id << "\r\n";
    
    for (const auto& header : headers) {
        oss << header.first << ": " << header.second << "\r\n";
    }
    oss << "\r\n";
    
    std::string header_str = oss.str();
    return std::vector<uint8_t>(header_str.begin(), header_str.end());
}

std::string PropertyManager::get_property_id_from_header(const std::vector<uint8_t>& header) const {
    std::string header_str(header.begin(), header.end());
    
    const std::string property_id_prefix = "X-Property-Id: ";
    size_t pos = header_str.find(property_id_prefix);
    if (pos != std::string::npos) {
        pos += property_id_prefix.length();
        size_t end_pos = header_str.find("\r\n", pos);
        if (end_pos != std::string::npos) {
            return header_str.substr(pos, end_pos - pos);
        }
    }
    
    return "";
}

} // namespace properties
} // namespace midi_ci
