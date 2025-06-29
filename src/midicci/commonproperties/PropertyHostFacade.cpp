#include "midicci/PropertyHostFacade.hpp"
#include "midicci/commonproperties/MidiCIServicePropertyRules.hpp"
#include "midicci/ObservablePropertyList.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/PropertyCommonRules.hpp"
#include "midicci/commonproperties/CommonRulesPropertyService.hpp"
#include "midicci/Json.hpp"
#include "midicci/MidiCIDevice.hpp"
#include "midicci/Message.hpp"
#include <mutex>
#include <algorithm>

namespace midicci {

class PropertyHostFacade::Impl {
public:
    explicit Impl(MidiCIDevice& device) : device_(device) {
        property_rules_ = std::make_unique<CommonRulesPropertyService>(device);
    }
    
    MidiCIDevice& device_;
    std::unique_ptr<MidiCIServicePropertyRules> property_rules_;
    std::vector<std::unique_ptr<PropertyMetadata>> properties_;
    PropertyHostFacade::PropertyUpdatedCallback property_updated_callback_;
    PropertyHostFacade::SubscriptionChangedCallback subscription_changed_callback_;
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
        auto reply = pimpl_->property_rules_->set_property_data(msg);
        
        auto status = pimpl_->property_rules_->get_header_field_integer(reply.get_header(), "status");
        if (status == 200) {
            auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.get_header());
            update_property(property_id, msg.get_body());
        }
        
        return reply;
    }
    
    return SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
}

SubscribePropertyReply PropertyHostFacade::process_subscribe_property(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto reply = pimpl_->property_rules_->subscribe_property(msg);
    if (reply.has_value()) {
        auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.get_header());
        auto command = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "command");

        if (command == "end") {
            auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
                [msg_source_muid = msg.get_source_muid(), &property_id](const PropertySubscription& sub) {
                    return sub.subscriber_muid == msg_source_muid && sub.property_id == property_id;
                });
            pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
            notify_subscription_changed(property_id);
        } else {
            PropertySubscription subscription;
            subscription.subscriber_muid = msg.get_source_muid();
            subscription.property_id = property_id;
            subscription.subscription_id = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "subscribeId");
            pimpl_->subscriptions_.push_back(subscription);
            notify_subscription_changed(property_id);
        }
        return std::move(reply.value());
    }
    else
        pimpl_->device_.get_logger()("Incoming SubscribeProperty message resulted in an error", true);
    return SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
}

void PropertyHostFacade::notify_property_updated(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_updated_callback_) {
        pimpl_->property_updated_callback_(property_id);
    }
}

void PropertyHostFacade::notify_subscription_changed(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->subscription_changed_callback_) {
        pimpl_->subscription_changed_callback_(property_id);
    }
}

void PropertyHostFacade::set_property_updated_callback(PropertyUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_updated_callback_ = std::move(callback);
}

void PropertyHostFacade::set_subscription_changed_callback(SubscriptionChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->subscription_changed_callback_ = std::move(callback);
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

SubscribeProperty PropertyHostFacade::createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, uint8_t group, uint8_t request_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto header = pimpl_->property_rules_->create_shutdown_subscription_header(property_id);
    
    Common common(pimpl_->device_.get_muid(), destination_muid, ADDRESS_FUNCTION_BLOCK, group);
    return {common, request_id, header, {}};
}

void PropertyHostFacade::shutdownSubscription(uint32_t destination_muid, const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    // Remove the subscription from the host's local list
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [destination_muid, &property_id](const PropertySubscription& sub) {
            return sub.subscriber_muid == destination_muid && sub.property_id == property_id;
        });
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
    notify_subscription_changed(property_id);

    auto& device = pimpl_->device_;
    auto msg = createShutdownSubscriptionMessage(destination_muid, property_id, device.get_config().group, device.get_messenger().get_next_request_id());
    device.get_messenger().send(msg);
}

} // namespace
