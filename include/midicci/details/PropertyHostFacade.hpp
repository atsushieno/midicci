#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "midicci/midicci.hpp"

namespace midicci {
    using namespace midicci::commonproperties;

struct PropertySubscription {
    uint32_t subscriber_muid;
    std::string property_id;
    std::string res_id;
    std::string subscription_id;
};

class PropertyHostFacade {
public:
    using PropertyUpdatedCallback = std::function<void(const std::string&, const std::string&)>;
    using SubscriptionChangedCallback = std::function<void(const std::string& property_id)>;
    
    explicit PropertyHostFacade(MidiCIDevice& device, MidiCIDeviceConfiguration& config);
    ~PropertyHostFacade();
    
    PropertyHostFacade(const PropertyHostFacade&) = delete;
    PropertyHostFacade& operator=(const PropertyHostFacade&) = delete;
    
    PropertyHostFacade(PropertyHostFacade&&) noexcept;
    PropertyHostFacade& operator=(PropertyHostFacade&&) noexcept;
    
    // Core property management (following Kotlin implementation)
    void addMetadata(std::unique_ptr<PropertyMetadata> property);
    void removeProperty(const std::string& property_id);
    void updatePropertyMetadata(const std::string& old_property_id, const PropertyMetadata& property);
    const PropertyMetadata* getPropertyMetadata(const std::string& property_id) const;
    
    // Property value updates with subscriber notifications (like Kotlin setPropertyValue)
    void setPropertyValue(const std::string& property_id, const std::string& res_id, const std::vector<uint8_t>& data, bool is_partial);
    
    // Common Rules updates (following Kotlin implementation)
    void updateCommonRulesDeviceInfo(const DeviceInfo& device_info);
    void updateCommonRulesChannelList(const MidiCIChannelList& channel_list);
    void updateJsonSchema(const std::string& json_schema);
    
    // Property service and rules management
    void setPropertyRules(std::unique_ptr<MidiCIServicePropertyRules> rules);
    MidiCIServicePropertyRules* getPropertyRules();
    
    // Observable property list access (following Kotlin lazy properties)
    ServiceObservablePropertyList& getProperties();
    const ServiceObservablePropertyList& getProperties() const;
    
    // Metadata list access (like Kotlin metadataList property) - returns safe pointers
    std::vector<const PropertyMetadata*> getMetadataList() const;
    
    // Message processing
    GetPropertyDataReply processGetPropertyData(const GetPropertyData& msg);
    SetPropertyDataReply processSetPropertyData(const SetPropertyData& msg);
    SubscribePropertyReply processSubscribeProperty(const SubscribeProperty& msg);
    
    // Subscription management
    std::vector<PropertySubscription> getSubscriptions() const;
    void shutdownSubscription(uint32_t subscriber_muid, const std::string& property_id, const std::string& res_id);
    void terminateSubscriptionsToAllSubscribers(uint8_t group);
    SubscribeProperty createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, const std::string& res_id, uint8_t group, uint8_t request_id);

    // Property binary getter accessor (following Kotlin propertyBinaryGetter)
    using PropertyBinaryGetter = std::function<std::vector<uint8_t>(const std::string& property_id, const std::string& res_id)>;
    void setPropertyBinaryGetter(PropertyBinaryGetter getter);
    PropertyBinaryGetter getPropertyBinaryGetter() const;

    // Property binary setter accessor (following Kotlin propertyBinarySetter)
    using PropertyBinarySetter = std::function<bool(const std::string& property_id, const std::string& res_id, const std::string& media_type, const std::vector<uint8_t>& body)>;
    void setPropertyBinarySetter(PropertyBinarySetter setter);
    PropertyBinarySetter getPropertyBinarySetter() const;

    // Notification callbacks
    void setPropertyUpdatedCallback(PropertyUpdatedCallback callback);
    void setSubscriptionChangedCallback(SubscriptionChangedCallback callback);
    void notifyPropertyUpdated(const std::string& property_id, const std::string& res_id);
    void notifySubscriptionChanged(const std::string& property_id);
    
    // Legacy compatibility helpers
    [[deprecated("Use removeProperty instead")]]
    void remove_property(const std::string& property_id) { removeProperty(property_id); }
    [[deprecated("Use setPropertyValue instead")]]
    void update_property(const std::string& property_id, const std::vector<uint8_t>& data) { setPropertyValue(property_id, "", data, false); }
    [[deprecated("Use updatePropertyMetadata instead")]]
    void update_property_metadata(const std::string& property_id, std::unique_ptr<PropertyMetadata> new_metadata) {
        if (new_metadata) {
            updatePropertyMetadata(property_id, *new_metadata);
        }
    }
    [[deprecated("Use getPropertyMetadata instead")]]
    const PropertyMetadata* get_property_metadata(const std::string& property_id) const { return getPropertyMetadata(property_id); }

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
