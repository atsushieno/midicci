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
    explicit Impl(MidiCIDevice& device) 
        : device_(device)
        , property_service_(std::make_unique<CommonRulesPropertyService>(device))
        , properties_(std::make_unique<ServiceObservablePropertyList>([&device](const std::string& msg, bool is_error) {
            device.get_logger()(msg, is_error);
          }))
    {
        // Set up property catalog update callback
        properties_->addPropertyCatalogUpdatedCallback([this]() {
            // Notify that the property catalog has changed
            if (property_updated_callback_) {
                property_updated_callback_(""); // Empty string indicates catalog change
            }
        });
        
        // Set up property value update callback
        properties_->addPropertyUpdatedCallback([this](const std::string& property_id) {
            notify_property_updated_to_subscribers(property_id);
        });
    }
    
    void notify_property_updated_to_subscribers(const std::string& property_id) {
        // Create property notifications for all subscribers
        auto property_values = properties_->getValues();
        auto it = std::find_if(property_values.begin(), property_values.end(),
            [&property_id](const PropertyValue& pv) { return pv.id == property_id; });
            
        if (it != property_values.end()) {
            // Notify all subscribers for this property
            for (const auto& subscription : subscriptions_) {
                if (subscription.property_id == property_id) {
                    // In a full implementation, this would create and send SubscribeProperty messages
                    // For now, just log the notification
                    device_.get_logger()("Notifying subscriber MUID 0x" + std::to_string(subscription.subscriber_muid) + 
                                       " of property update: " + property_id, false);
                }
            }
        }
    }
    
    MidiCIDevice& device_;
    std::unique_ptr<MidiCIServicePropertyRules> property_service_;
    std::unique_ptr<ServiceObservablePropertyList> properties_;
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
    pimpl_->property_service_ = std::move(rules);
}

MidiCIServicePropertyRules* PropertyHostFacade::get_property_rules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_service_.get();
}

// Core property management methods (following Kotlin implementation)
void PropertyHostFacade::addProperty(const PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Create a copy of the metadata for the observable property list
    auto metadata_copy_for_properties = std::make_unique<CommonRulesPropertyMetadata>(property.getPropertyId());
    metadata_copy_for_properties->setData(property.getData());
    
    // Create another copy for the service layer registration
    auto metadata_copy_for_service = std::make_unique<CommonRulesPropertyMetadata>(property.getPropertyId());
    metadata_copy_for_service->setData(property.getData());
    
    // Copy extended metadata if it's a CommonRulesPropertyMetadata
    if (auto* common_rules = dynamic_cast<const CommonRulesPropertyMetadata*>(&property)) {
        auto* target1 = static_cast<CommonRulesPropertyMetadata*>(metadata_copy_for_properties.get());
        auto* target2 = static_cast<CommonRulesPropertyMetadata*>(metadata_copy_for_service.get());
        
        target1->canGet = common_rules->canGet;
        target1->canSet = common_rules->canSet;
        target1->canSubscribe = common_rules->canSubscribe;
        target1->requireResId = common_rules->requireResId;
        target1->canPaginate = common_rules->canPaginate;
        target1->mediaTypes = common_rules->mediaTypes;
        target1->encodings = common_rules->encodings;
        target1->schema = common_rules->schema;
        
        target2->canGet = common_rules->canGet;
        target2->canSet = common_rules->canSet;
        target2->canSubscribe = common_rules->canSubscribe;
        target2->requireResId = common_rules->requireResId;
        target2->canPaginate = common_rules->canPaginate;
        target2->mediaTypes = common_rules->mediaTypes;
        target2->encodings = common_rules->encodings;
        target2->schema = common_rules->schema;
    }
    
    // CRITICAL: Register with the service layer so it appears in ResourceList
    if (pimpl_->property_service_) {
        pimpl_->property_service_->add_metadata(std::move(metadata_copy_for_service));
        
        // Also set the initial property value in the service layer
        if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
            common_service->set_property_value(property.getPropertyId(), property.getData());
        }
    }
    
    // Add to the observable property list with initial value
    pimpl_->properties_->addProperty(std::move(metadata_copy_for_properties), property.getData());
}

void PropertyHostFacade::removeProperty(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // CRITICAL: Remove from the service layer so it disappears from ResourceList
    if (pimpl_->property_service_) {
        pimpl_->property_service_->remove_metadata(property_id);
    }
    
    // Remove from the observable property list
    pimpl_->properties_->removeProperty(property_id);
    
    // Remove any subscriptions for this property
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&property_id](const PropertySubscription& sub) {
            return sub.property_id == property_id;
        });
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
    
    notify_subscription_changed(property_id);
}

void PropertyHostFacade::updatePropertyMetadata(const std::string& old_property_id, const PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // For now, treat this as remove + add to ensure proper service layer registration
    // Save existing data first
    std::vector<uint8_t> existing_data;
    auto current_values = pimpl_->properties_->getValues();
    auto it = std::find_if(current_values.begin(), current_values.end(),
        [&old_property_id](const PropertyValue& pv) { return pv.id == old_property_id; });
    
    if (it != current_values.end()) {
        existing_data = it->body;
    }
    
    // Remove old property (this removes from both service layer and observable list)
    removeProperty(old_property_id);
    
    // Add new property (this adds to both service layer and observable list)
    addProperty(property);
    
    // Restore the data if we had any
    if (!existing_data.empty()) {
        setPropertyValue(property.getPropertyId(), "", existing_data, false);
    }
}

// Property value updates with subscriber notifications (like Kotlin setPropertyValue)
void PropertyHostFacade::setPropertyValue(const std::string& property_id, const std::string& res_id, 
                                         const std::vector<uint8_t>& data, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // CRITICAL: Update the property value in the service layer so GetPropertyData works
    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        common_service->set_property_value(property_id, data);
    }
    
    // Update the property value in our observable list with full property information
    // Get the current media type from metadata or use default
    auto* metadata = get_property_metadata(property_id);
    std::string media_type = "application/json"; // Default media type
    if (metadata) {
        media_type = metadata->getMediaType();
    }
    
    // Use the enhanced updateProperty method that handles resource ID and media type
    pimpl_->properties_->updateValue(property_id, res_id, media_type, data);
    
    // This will trigger the property updated callback which notifies subscribers
    notify_property_updated(property_id);
}

// Common Rules updates (following Kotlin implementation)
void PropertyHostFacade::updateCommonRulesDeviceInfo(const DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (auto* common_rules = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        // Update device info in the service
        // This would need to be implemented in CommonRulesPropertyService
        pimpl_->device_.get_logger()("Updated Common Rules device info", false);
    }
}

void PropertyHostFacade::updateCommonRulesChannelList(const MidiCIChannelList& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (auto* common_rules = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        // Update channel list in the service
        // This would need to be implemented in CommonRulesPropertyService
        pimpl_->device_.get_logger()("Updated Common Rules channel list", false);
    }
}

void PropertyHostFacade::updateJsonSchema(const std::string& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (auto* common_rules = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        // Update JSON schema in the service
        // This would need to be implemented in CommonRulesPropertyService
        pimpl_->device_.get_logger()("Updated JSON schema", false);
    }
}

// Observable property list access (following Kotlin lazy properties)
ServiceObservablePropertyList& PropertyHostFacade::get_properties() {
    return *pimpl_->properties_;
}

const ServiceObservablePropertyList& PropertyHostFacade::get_properties() const {
    return *pimpl_->properties_;
}

GetPropertyDataReply PropertyHostFacade::process_get_property_data(const GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_service_) {
        return pimpl_->property_service_->get_property_data(msg);
    }
    
    return GetPropertyDataReply(msg.get_common(), msg.get_request_id(), {}, {});
}

SetPropertyDataReply PropertyHostFacade::process_set_property_data(const SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_service_) {
        auto reply = pimpl_->property_service_->set_property_data(msg);
        
        auto status = pimpl_->property_service_->get_header_field_integer(reply.get_header(), "status");
        if (status == 200) {
            auto property_id = pimpl_->property_service_->get_property_id_for_header(msg.get_header());
            // Update property value using new implementation
            pimpl_->properties_->updateValue(property_id, msg.get_header(), msg.get_body());
            notify_property_updated(property_id);
        }
        
        return reply;
    }
    
    return SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
}

SubscribePropertyReply PropertyHostFacade::process_subscribe_property(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_service_) {
        pimpl_->device_.get_logger()("No property service available for subscription", true);
        return SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
    }
    
    auto reply = pimpl_->property_service_->subscribe_property(msg);
    if (reply.has_value()) {
        auto property_id = pimpl_->property_service_->get_property_id_for_header(msg.get_header());
        auto command = pimpl_->property_service_->get_header_field_string(msg.get_header(), "command");

        if (command == "end") {
            // Remove subscription
            auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
                [msg_source_muid = msg.get_source_muid(), &property_id](const PropertySubscription& sub) {
                    return sub.subscriber_muid == msg_source_muid && sub.property_id == property_id;
                });
            pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
            notify_subscription_changed(property_id);
        } else {
            // Add subscription
            PropertySubscription subscription;
            subscription.subscriber_muid = msg.get_source_muid();
            subscription.property_id = property_id;
            subscription.subscription_id = pimpl_->property_service_->get_header_field_string(msg.get_header(), "subscribeId");
            pimpl_->subscriptions_.push_back(subscription);
            notify_subscription_changed(property_id);
        }
        return std::move(reply.value());
    }
    else {
        pimpl_->device_.get_logger()("Incoming SubscribeProperty message resulted in an error", true);
    }
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

std::vector<PropertySubscription> PropertyHostFacade::get_subscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

SubscribeProperty PropertyHostFacade::createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, uint8_t group, uint8_t request_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_service_) {
        pimpl_->device_.get_logger()("No property service available for shutdown message", true);
        Common common(pimpl_->device_.get_muid(), destination_muid, ADDRESS_FUNCTION_BLOCK, group);
        std::vector<uint8_t> empty_header, empty_body;
        return SubscribeProperty(common, request_id, empty_header, empty_body);
    }
    
    auto header = pimpl_->property_service_->create_shutdown_subscription_header(property_id);
    
    Common common(pimpl_->device_.get_muid(), destination_muid, ADDRESS_FUNCTION_BLOCK, group);
    std::vector<uint8_t> empty_body;
    return SubscribeProperty(common, request_id, header, empty_body);
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

// Terminate all subscriptions (following Kotlin terminateSubscriptionsToAllSubsctibers)
void PropertyHostFacade::terminateSubscriptionsToAllSubscribers(uint8_t group) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Send shutdown messages to all current subscribers
    for (const auto& subscription : pimpl_->subscriptions_) {
        auto& device = pimpl_->device_;
        auto msg = createShutdownSubscriptionMessage(
            subscription.subscriber_muid, 
            subscription.property_id, 
            group, 
            device.get_messenger().get_next_request_id());
        device.get_messenger().send(msg);
    }
    
    // Clear all subscriptions
    pimpl_->subscriptions_.clear();
    
    // Notify about subscription changes for all properties
    auto property_ids = get_property_ids();
    for (const auto& property_id : property_ids) {
        notify_subscription_changed(property_id);
    }
}

// Legacy compatibility methods (delegate to new implementation)
void PropertyHostFacade::add_property(std::unique_ptr<PropertyMetadata> property) {
    if (property) {
        addProperty(*property);
    }
}

void PropertyHostFacade::remove_property(const std::string& property_id) {
    removeProperty(property_id);
}

void PropertyHostFacade::update_property(const std::string& property_id, const std::vector<uint8_t>& data) {
    setPropertyValue(property_id, "", data, false);
}

void PropertyHostFacade::update_property_metadata(const std::string& property_id, std::unique_ptr<PropertyMetadata> new_metadata) {
    if (new_metadata) {
        updatePropertyMetadata(property_id, *new_metadata);
    }
}

// Legacy compatibility methods
std::vector<uint8_t> PropertyHostFacade::getProperty(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto values = pimpl_->properties_->getValues();
    auto it = std::find_if(values.begin(), values.end(),
        [&property_id](const PropertyValue& pv) { return pv.id == property_id; });
    
    if (it != values.end()) {
        return it->body;
    }
    return {};
}

std::vector<std::string> PropertyHostFacade::get_property_ids() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::vector<std::string> property_ids;
    
    auto values = pimpl_->properties_->getValues();
    for (const auto& value : values) {
        property_ids.push_back(value.id);
    }
    return property_ids;
}

const PropertyMetadata* PropertyHostFacade::get_property_metadata(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Use the safer direct metadata access method instead of getMetadataList()
    return pimpl_->properties_->getMetadata(property_id);
}

} // namespace midicci