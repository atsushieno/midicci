#include "midicci/PropertyHostFacade.hpp"
#include "midicci/commonproperties/MidiCIServicePropertyRules.hpp"
#include "midicci/ObservablePropertyList.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/PropertyCommonRules.hpp"
#include "midicci/commonproperties/CommonRulesPropertyService.hpp"
#include "midicci/Json.hpp"
#include "midicci/MidiCIDevice.hpp"
#include "midicci/Message.hpp"
#include "midicci/MidiCIConstants.hpp"
#include <mutex>
#include <algorithm>

namespace midicci {

class PropertyHostFacade::Impl {
public:
    explicit Impl(MidiCIDevice& device, MidiCIDeviceConfiguration& config)
        : device_(device)
        , property_service_(std::make_unique<CommonRulesPropertyService>(device))
    {
        properties_ = std::make_unique<ServiceObservablePropertyList>(config.property_values, *property_service_);

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
        // Get the property value
        auto property_values = properties_->getValues();
        auto it = std::find_if(property_values.begin(), property_values.end(),
            [&property_id](const PropertyValue& pv) { return pv.id == property_id; });
            
        if (it != property_values.end()) {
            // Call the main notification method with property data and isPartial=false
            notify_property_updates_to_subscribers(property_id, it->body, false);
        }
    }
    
    // Main notification method that matches Kotlin implementation
    void notify_property_updates_to_subscribers(const std::string& property_id, const std::vector<uint8_t>& data, bool is_partial) {
        create_property_notification(property_id, data, is_partial);
    }
    
    // Create and send property notification messages (port of Kotlin createPropertyNotification)
    void create_property_notification(const std::string& property_id, const std::vector<uint8_t>& data, bool is_partial) {
        std::string last_encoding;
        std::vector<uint8_t> last_encoded_data = data;
        
        // Get subscriptions from property service
        const auto& service_subscriptions = property_service_->get_subscriptions();
        
        for (const auto& subscription : service_subscriptions) {
            if (subscription.resource != property_id) {
                continue; // Skip subscriptions for other properties
            }
            
            // Encode data if needed (optimize by caching last encoding)
            std::vector<uint8_t> encoded_data;
            if (subscription.encoding == last_encoding) {
                encoded_data = last_encoded_data;
            } else if (subscription.encoding.empty()) {
                encoded_data = data; // No encoding needed
            } else {
                encoded_data = property_service_->encode_body(data, subscription.encoding);
                // Cache the encoding result
                last_encoding = subscription.encoding;
                last_encoded_data = encoded_data;
            }
            
            // Create header fields for notification
            std::map<std::string, std::string> header_fields;
            header_fields[PropertyCommonHeaderKeys::SUBSCRIBE_ID] = subscription.subscribe_id;
            header_fields[PropertyCommonHeaderKeys::SET_PARTIAL] = is_partial ? "true" : "false";
            if (!subscription.encoding.empty()) {
                header_fields[PropertyCommonHeaderKeys::MUTUAL_ENCODING] = subscription.encoding;
            }
            
            // Create notification header
            auto header = property_service_->create_update_notification_header(subscription.resource, header_fields);
            
            // Create and send SubscribeProperty message
            Common common(device_.get_muid(), BROADCAST_MUID_32, ADDRESS_FUNCTION_BLOCK, device_.get_config().group);
            auto request_id = device_.get_messenger().get_next_request_id();
            SubscribeProperty msg(common, request_id, header, encoded_data);
            
            // Send the notification
            device_.get_messenger().send(msg);
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

PropertyHostFacade::PropertyHostFacade(MidiCIDevice& device, MidiCIDeviceConfiguration& config) {
    pimpl_ = std::make_unique<Impl>(device, config);
}

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
void PropertyHostFacade::addMetadata(std::unique_ptr<PropertyMetadata> property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    get_properties().addMetadata(std::move(property));
}

void PropertyHostFacade::removeProperty(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // CRITICAL: Remove from the service layer so it disappears from ResourceList
    if (pimpl_->property_service_) {
        pimpl_->property_service_->remove_metadata(property_id);
    }
    
    // Remove from the observable property list
    pimpl_->properties_->removeMetadata(property_id);
    
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

    pimpl_->properties_->updateMetadata(old_property_id, (PropertyMetadata*) &property);
}

// Property value updates with subscriber notifications (matching Kotlin setPropertyValue)
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
    
    // Directly notify subscribers with the data and partial flag (like Kotlin)
    pimpl_->notify_property_updates_to_subscribers(property_id, data, is_partial);
    
    // Also trigger the standard property updated callback
    notify_property_updated(property_id);
}

// Common Rules updates (following Kotlin implementation)
void PropertyHostFacade::updateCommonRulesDeviceInfo(const DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update device info in the device itself
    auto& current_device_info = pimpl_->device_.get_device_info();
    current_device_info = device_info;
    
    // Notify that DeviceInfo property may have changed
    notify_property_updated(PropertyResourceNames::DEVICE_INFO);
}

void PropertyHostFacade::updateCommonRulesChannelList(const MidiCIChannelList& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update channel list in the device configuration
    pimpl_->device_.get_config().channel_list = channel_list;
    
    // Notify that ChannelList property may have changed
    notify_property_updated(PropertyResourceNames::CHANNEL_LIST);
}

void PropertyHostFacade::updateJsonSchema(const std::string& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update JSON schema in the device configuration
    pimpl_->device_.get_config().json_schema_string = json_schema;
    
    // Notify that JSONSchema property may have changed
    notify_property_updated(PropertyResourceNames::JSON_SCHEMA);
}

// Observable property list access (following Kotlin lazy properties)
ServiceObservablePropertyList& PropertyHostFacade::get_properties() {
    return *pimpl_->properties_;
}

const ServiceObservablePropertyList& PropertyHostFacade::get_properties() const {
    return *pimpl_->properties_;
}

// Metadata list access (like Kotlin metadataList property) - returns safe pointers
std::vector<const PropertyMetadata*> PropertyHostFacade::get_metadata_list() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    std::vector<const PropertyMetadata*> result;
    
    // Get all property IDs and use the safe getMetadata method
    auto property_ids = get_property_ids();
    for (const auto& property_id : property_ids) {
        const PropertyMetadata* metadata = pimpl_->properties_->getMetadata(property_id);
        if (metadata) {
            result.push_back(metadata);
        }
    }
    
    return result;
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
    if (property)
        get_properties().addMetadata(std::move(property));
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
    
    auto metadata_list = pimpl_->properties_->getMetadataList();
    for (const auto& metadata : metadata_list) {
        if (metadata) {
            property_ids.push_back(metadata->getPropertyId());
        }
    }
    return property_ids;
}

const PropertyMetadata* PropertyHostFacade::get_property_metadata(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Use the safer direct metadata access method instead of getMetadataList()
    return pimpl_->properties_->getMetadata(property_id);
}

} // namespace midicci