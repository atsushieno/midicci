#include "midi-ci/properties/PropertyHostFacade.hpp"
#include "midi-ci/properties/MidiCIServicePropertyRules.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/messages/Message.hpp"
#include <mutex>
#include <algorithm>

namespace midi_ci {
namespace properties {

class PropertyHostFacade::Impl {
public:
    explicit Impl(core::MidiCIDevice& device) : device_(device) {}
    
    core::MidiCIDevice& device_;
    std::unique_ptr<MidiCIServicePropertyRules> property_rules_;
    std::vector<PropertyMetadata> properties_;
    mutable std::recursive_mutex mutex_;
};

PropertyHostFacade::PropertyHostFacade(core::MidiCIDevice& device) : pimpl_(std::make_unique<Impl>(device)) {}

PropertyHostFacade::~PropertyHostFacade() = default;

void PropertyHostFacade::set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_rules_ = std::move(rules);
}

MidiCIServicePropertyRules* PropertyHostFacade::get_property_rules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_rules_.get();
}

void PropertyHostFacade::add_property(const PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property](const PropertyMetadata& p) {
            return p.property_id == property.property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        *it = property;
    } else {
        pimpl_->properties_.push_back(property);
    }
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->add_metadata(property);
    }
}

void PropertyHostFacade::remove_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const PropertyMetadata& p) {
            return p.property_id == property_id;
        });
    
    pimpl_->properties_.erase(it, pimpl_->properties_.end());
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->remove_metadata(property_id);
    }
}

void PropertyHostFacade::update_property(const std::string& property_id, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->properties_.begin(), pimpl_->properties_.end(),
        [&property_id](const PropertyMetadata& p) {
            return p.property_id == property_id;
        });
    
    if (it != pimpl_->properties_.end()) {
        it->data = data;
        notify_property_updated(property_id);
    }
}

messages::GetPropertyDataReply PropertyHostFacade::process_get_property_data(const messages::GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->get_property_data(msg);
    }
    
    return messages::GetPropertyDataReply(msg.get_common(), msg.get_request_id(), {}, {});
}

messages::SetPropertyDataReply PropertyHostFacade::process_set_property_data(const messages::SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->set_property_data(msg);
    }
    
    return messages::SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
}

messages::SubscribePropertyReply PropertyHostFacade::process_subscribe_property(const messages::SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        return pimpl_->property_rules_->subscribe_property(msg);
    }
    
    return messages::SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
}

void PropertyHostFacade::notify_property_updated(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

} // namespace properties
} // namespace midi_ci
