#include "midicci/properties/PropertyHostFacade.hpp"
#include "midicci/properties/MidiCIServicePropertyRules.hpp"
#include "midicci/properties/ObservablePropertyList.hpp"
#include "midicci/properties/CommonRulesPropertyMetadata.hpp"
#include "midicci/properties/PropertyCommonRules.hpp"
#include "midicci/properties/CommonRulesPropertyService.hpp"
#include "midicci/json_ish/Json.hpp"
#include "midicci/core/MidiCIDevice.hpp"
#include "midicci/messages/Message.hpp"
#include <mutex>
#include <algorithm>

namespace midicci {
namespace propertycommonrules {

class PropertyHostFacade::Impl {
public:
    explicit Impl(MidiCIDevice& device) : device_(device) {
        property_rules_ = std::make_unique<CommonRulesPropertyService>(device);
    }
    
    MidiCIDevice& device_;
    std::unique_ptr<MidiCIServicePropertyRules> property_rules_;
    std::vector<std::unique_ptr<PropertyMetadata>> properties_;
    PropertyHostFacade::PropertyUpdatedCallback property_updated_callback_;
    std::vector<PropertySubscription> subscriptions_;
    mutable std::recursive_mutex mutex_;
};

PropertyHostFacade::PropertyHostFacade(MidiCIDevice& device) : pimpl_(std::make_unique<Impl>(device)) {}

PropertyHostFacade::~PropertyHostFacade() = default;

PropertyHostFacade::PropertyHostFacade(PropertyHostFacade&&) noexcept = default;

PropertyHostFacade& PropertyHostFacade::operator=(PropertyHostFacade&&) noexcept = default;

void PropertyHostFacade::set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_rules_ = std::move(rules);
}

MidiCIServicePropertyRules* PropertyHostFacade::get_property_rules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_rules_.get();
}

void PropertyHostFacade::add_property(std::unique_ptr<PropertyMetadata> property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property->getPropertyId();
        });
    
    if (it != pimpl_->properties_.end()) {
        *it = std::move(property);
    } else {
        pimpl_->properties_.push_back(std::move(property));
    }
}

void PropertyHostFacade::remove_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property_id;
        });
    
    pimpl_->properties_.erase(it, pimpl_->properties_.end());
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->remove_metadata(property_id);
    }
}

void PropertyHostFacade::update_property(const std::string& property_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        if (auto* common_rules = dynamic_cast<CommonRulesPropertyMetadata*>(it->get())) {
            common_rules->setData(data);
        }
        notify_property_updated(property_id);
    }
}

GetPropertyDataReply PropertyHostFacade::process_get_property_data(const GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->get_property_data(msg);
    }
    
    return GetPropertyDataReply(msg.get_common(), msg.get_request_id(), {}, {});
}

SetPropertyDataReply PropertyHostFacade::process_set_property_data(const SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->set_property_data(msg);
    }
    
    return SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
}

SubscribePropertyReply PropertyHostFacade::process_subscribe_property(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->subscribe_property(msg);
    }
    
    return SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
}

void PropertyHostFacade::notify_property_updated(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_updated_callback_) {
        pimpl_->property_updated_callback_(property_id);
    }
}

void PropertyHostFacade::set_property_updated_callback(PropertyUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_updated_callback_ = std::move(callback);
}

void PropertyHostFacade::setPropertyValue(const std::string& property_id, const std::string& res_id, const std::vector<uint8_t>& data, bool notify) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        if (auto* common_rules = dynamic_cast<CommonRulesPropertyMetadata*>(it->get())) {
            common_rules->setData(data);
        }
        if (notify) {
            notify_property_updated(property_id);
        }
    }
}

std::vector<uint8_t> PropertyHostFacade::getProperty(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        return (*it)->getData();
    }
    return {};
}

std::vector<PropertySubscription> PropertyHostFacade::get_subscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

void PropertyHostFacade::shutdownSubscription(uint32_t subscriber_muid, const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [subscriber_muid, &property_id](const PropertySubscription& sub) {
            return sub.subscriber_muid == subscriber_muid && sub.property_id == property_id;
        });
    
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
}

} // namespace properties
} // namespace midi_ci
