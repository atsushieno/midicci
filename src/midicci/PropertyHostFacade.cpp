#include "midicci/midicci.hpp"
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
                property_updated_callback_("", "");
            }
        });
        
        // Set up property value update callback
        properties_->addPropertyUpdatedCallback([this](const std::string& property_id, const std::string& res_id) {
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
        const auto& service_subscriptions = property_service_->getSubscriptions();
        
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
                encoded_data = property_service_->encodeBody(data, subscription.encoding);
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
            auto header = property_service_->createUpdateNotificationHeader(subscription.resource, header_fields);
            
            // Create and send SubscribeProperty message
            Common common(device_.getMuid(), BROADCAST_MUID_32, ADDRESS_FUNCTION_BLOCK, device_.getConfig().group);
            auto request_id = device_.getMessenger().getNextRequestId();
            SubscribeProperty msg(common, request_id, header, encoded_data);
            
            // Send the notification
            device_.getMessenger().send(msg);
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

void PropertyHostFacade::setPropertyRules(std::unique_ptr<MidiCIServicePropertyRules> rules) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_service_ = std::move(rules);
}

MidiCIServicePropertyRules* PropertyHostFacade::getPropertyRules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_service_.get();
}

// Core property management methods (following Kotlin implementation)
void PropertyHostFacade::addMetadata(std::unique_ptr<PropertyMetadata> property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    getProperties().addMetadata(std::move(property));
}

void PropertyHostFacade::removeProperty(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // CRITICAL: Remove from the service layer so it disappears from ResourceList
    if (pimpl_->property_service_) {
        pimpl_->property_service_->removeMetadata(property_id);
    }
    
    // Remove from the observable property list
    pimpl_->properties_->removeMetadata(property_id);
    
    // Remove any subscriptions for this property
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&property_id](const PropertySubscription& sub) {
            return sub.property_id == property_id;
        });
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
    
    notifySubscriptionChanged(property_id);
}

void PropertyHostFacade::updatePropertyMetadata(const std::string& old_property_id, const PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    pimpl_->properties_->updateMetadata(old_property_id, (PropertyMetadata*) &property);
}

// Property value updates with subscriber notifications (matching Kotlin setPropertyValue)
void PropertyHostFacade::setPropertyValue(const std::string& property_id, const std::string& res_id, 
                                         const std::vector<uint8_t>& data, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Following Kotlin implementation exactly: properties.values.first { it.id == propertyId && (resId == null || it.resId == resId) }.body = data
    auto& property_values = pimpl_->properties_->getMutableValues();
    std::string media_type = CommonRulesKnownMimeTypes::APPLICATION_JSON;
    auto it = std::find_if(property_values.begin(), property_values.end(),
        [&property_id, &res_id](const PropertyValue& pv) {
            return pv.id == property_id && pv.resId == res_id;
        });
    
    if (it != property_values.end()) {
        // Update existing property value directly (following Kotlin pattern)
        it->body = data;
        media_type = it->mediaType;
    } else {
        auto* metadata = getPropertyMetadata(property_id);
        if (metadata) {
            auto* common_rules_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata);
            if (common_rules_metadata && !common_rules_metadata->mediaTypes.empty()) {
                media_type = common_rules_metadata->mediaTypes[0];
            }
        }

        property_values.emplace_back(property_id, res_id, media_type, data);
    }
    
    // CRITICAL: Update the property value in the service layer so GetPropertyData works
    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        common_service->setPropertyValue(property_id, res_id, data, media_type);
    }
    
    // Following Kotlin implementation: notifyPropertyUpdatesToSubscribers(propertyId, data, isPartial)
    pimpl_->notify_property_updates_to_subscribers(property_id, data, is_partial);
}

// Common Rules updates (following Kotlin implementation)
void PropertyHostFacade::updateCommonRulesDeviceInfo(const DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update device info in the device itself
    auto& current_device_info = pimpl_->device_.getDeviceInfo();
    current_device_info = device_info;

    // Notify that DeviceInfo property may have changed
    notifyPropertyUpdated(PropertyResourceNames::DEVICE_INFO, "");
}

void PropertyHostFacade::updateCommonRulesChannelList(const MidiCIChannelList& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update channel list in the device configuration
    pimpl_->device_.getConfig().channel_list = channel_list;

    // Notify that ChannelList property may have changed
    notifyPropertyUpdated(PropertyResourceNames::CHANNEL_LIST, "");
}

void PropertyHostFacade::updateJsonSchema(const std::string& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    // Update JSON schema in the device configuration
    pimpl_->device_.getConfig().json_schema_string = json_schema;

    // Notify that JSONSchema property may have changed
    notifyPropertyUpdated(PropertyResourceNames::JSON_SCHEMA, "");
}

// Observable property list access (following Kotlin lazy properties)
ServiceObservablePropertyList& PropertyHostFacade::getProperties() {
    return *pimpl_->properties_;
}

const ServiceObservablePropertyList& PropertyHostFacade::getProperties() const {
    return *pimpl_->properties_;
}

// Metadata list access (like Kotlin metadataList property) - returns safe pointers
std::vector<const PropertyMetadata*> PropertyHostFacade::getMetadataList() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    std::vector<const PropertyMetadata*> result;
    
    // Get metadata directly from the list
    auto metadata_list = pimpl_->properties_->getMetadataList();
    for (const auto& metadata : metadata_list) {
        if (metadata) {
            result.push_back(metadata.get());
        }
    }
    
    return result;
}

GetPropertyDataReply PropertyHostFacade::processGetPropertyData(const GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_service_) {
        return pimpl_->property_service_->getPropertyData(msg);
    }
    
    return GetPropertyDataReply(msg.getCommon(), msg.getRequestId(), {}, {});
}

SetPropertyDataReply PropertyHostFacade::processSetPropertyData(const SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_service_) {
        auto reply = pimpl_->property_service_->setPropertyData(msg);
        
        // Following Kotlin implementation: check if reply is successful, then update property
        auto status = pimpl_->property_service_->getHeaderFieldInteger(reply.getHeader(), PropertyCommonHeaderKeys::STATUS);
        if (status == PropertyExchangeStatus::OK) {
            pimpl_->properties_->updateValue(msg.getHeader(), msg.getBody());
            
            // Note: Don't call notifyPropertyUpdated here as it's already called by updateValue
        }
        
        return reply;
    }
    
    return SetPropertyDataReply(msg.getCommon(), msg.getRequestId(), {});
}

SubscribePropertyReply PropertyHostFacade::processSubscribeProperty(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_service_) {
        pimpl_->device_.getLogger()(LogData("No property service available for subscription", true));
        return SubscribePropertyReply(msg.getCommon(), msg.getRequestId(), {}, {});
    }
    
    auto reply = pimpl_->property_service_->subscribeProperty(msg);
    if (reply.has_value()) {
        auto property_id = pimpl_->property_service_->getPropertyIdForHeader(msg.getHeader());
        auto command = pimpl_->property_service_->getHeaderFieldString(msg.getHeader(), "command");

        if (command == "end") {
            // Remove subscription
            auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
                [msg_source_muid = msg.getSourceMuid(), &property_id](const PropertySubscription& sub) {
                    return sub.subscriber_muid == msg_source_muid && sub.property_id == property_id;
                });
            pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
            notifySubscriptionChanged(property_id);
        } else {
            // Add subscription
            PropertySubscription subscription;
            subscription.subscriber_muid = msg.getSourceMuid();
            subscription.property_id = property_id;
            subscription.subscription_id = pimpl_->property_service_->getHeaderFieldString(msg.getHeader(), "subscribeId");
            pimpl_->subscriptions_.push_back(subscription);
            notifySubscriptionChanged(property_id);
        }
        return std::move(reply.value());
    }
    else {
        pimpl_->device_.getLogger()(LogData("Incoming SubscribeProperty message resulted in an error", true));
    }
    return SubscribePropertyReply(msg.getCommon(), msg.getRequestId(), {}, {});
}

void PropertyHostFacade::notifyPropertyUpdated(const std::string& property_id, const std::string& res_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    if (pimpl_->property_updated_callback_) {
        pimpl_->property_updated_callback_(property_id, res_id);
    }
}

void PropertyHostFacade::notifySubscriptionChanged(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->subscription_changed_callback_) {
        pimpl_->subscription_changed_callback_(property_id);
    }
}

void PropertyHostFacade::setPropertyUpdatedCallback(PropertyUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_updated_callback_ = std::move(callback);
}

void PropertyHostFacade::setSubscriptionChangedCallback(SubscriptionChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->subscription_changed_callback_ = std::move(callback);
}

std::vector<PropertySubscription> PropertyHostFacade::getSubscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

SubscribeProperty PropertyHostFacade::createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, const std::string& res_id, uint8_t group, uint8_t request_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_service_) {
        pimpl_->device_.getLogger()(LogData("No property service available for shutdown message", true));
        Common common(pimpl_->device_.getMuid(), destination_muid, ADDRESS_FUNCTION_BLOCK, group);
        std::vector<uint8_t> empty_header, empty_body;
        return SubscribeProperty(common, request_id, empty_header, empty_body);
    }
    
    auto header = pimpl_->property_service_->createShutdownSubscriptionHeader(property_id, res_id);
    
    Common common(pimpl_->device_.getMuid(), destination_muid, ADDRESS_FUNCTION_BLOCK, group);
    std::vector<uint8_t> empty_body;
    return SubscribeProperty(common, request_id, header, empty_body);
}

void PropertyHostFacade::shutdownSubscription(uint32_t destination_muid, const std::string& property_id, const std::string& res_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    // Remove the subscription from the host's local list
    auto it = std::remove_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [destination_muid, &property_id, &res_id](const PropertySubscription& sub) {
            return sub.subscriber_muid == destination_muid && sub.property_id == property_id && (res_id.empty() || sub.res_id == res_id);
        });
    pimpl_->subscriptions_.erase(it, pimpl_->subscriptions_.end());
    notifySubscriptionChanged(property_id);

    auto& device = pimpl_->device_;
    auto msg = createShutdownSubscriptionMessage(destination_muid, property_id, res_id, device.getConfig().group, device.getMessenger().getNextRequestId());
    device.getMessenger().send(msg);
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
            subscription.res_id, 
            group, 
            device.getMessenger().getNextRequestId());
        device.getMessenger().send(msg);
    }
    
    // Clear all subscriptions
    pimpl_->subscriptions_.clear();
    
    // Notify about subscription changes for all properties
    auto metadata_list = pimpl_->properties_->getMetadataList();
    for (const auto& metadata : metadata_list) {
        if (metadata) {
            notifySubscriptionChanged(metadata->getPropertyId());
        }
    }
}

const PropertyMetadata* PropertyHostFacade::getPropertyMetadata(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    // Use the safer direct metadata access method instead of getMetadataList()
    return pimpl_->properties_->getMetadata(property_id);
}

void PropertyHostFacade::setPropertyBinaryGetter(PropertyBinaryGetter getter) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        common_service->propertyBinaryGetter = std::move(getter);
    }
}

PropertyHostFacade::PropertyBinaryGetter PropertyHostFacade::getPropertyBinaryGetter() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        return common_service->propertyBinaryGetter;
    }

    // Return default empty lambda
    return [](const std::string&, const std::string&) -> std::vector<uint8_t> { return {}; };
}

void PropertyHostFacade::setPropertyBinarySetter(PropertyBinarySetter setter) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        common_service->propertyBinarySetter = std::move(setter);
    }
}

PropertyHostFacade::PropertyBinarySetter PropertyHostFacade::getPropertyBinarySetter() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    if (auto* common_service = dynamic_cast<CommonRulesPropertyService*>(pimpl_->property_service_.get())) {
        return common_service->propertyBinarySetter;
    }

    // Return default lambda that always fails
    return [](const std::string&, const std::string&, const std::string&, const std::vector<uint8_t>&) -> bool { return false; };
}

} // namespace midicci
