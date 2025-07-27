#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "Message.hpp"
#include "ObservablePropertyList.hpp"
#include "commonproperties/MidiCIServicePropertyRules.hpp"
#include "MidiCIDevice.hpp"

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
    using PropertyUpdatedCallback = std::function<void(const std::string&)>;
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
    
    // Property value updates with subscriber notifications (like Kotlin setPropertyValue)
    void setPropertyValue(const std::string& property_id, const std::string& res_id, const std::vector<uint8_t>& data, bool is_partial);
    
    // Common Rules updates (following Kotlin implementation)
    void updateCommonRulesDeviceInfo(const DeviceInfo& device_info);
    void updateCommonRulesChannelList(const MidiCIChannelList& channel_list);
    void updateJsonSchema(const std::string& json_schema);
    
    // Property service and rules management
    void set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules);
    MidiCIServicePropertyRules* get_property_rules();
    
    // Observable property list access (following Kotlin lazy properties)
    ServiceObservablePropertyList& get_properties();
    const ServiceObservablePropertyList& get_properties() const;
    
    // Metadata list access (like Kotlin metadataList property) - returns safe pointers
    std::vector<const PropertyMetadata*> get_metadata_list() const;
    
    // Message processing
    GetPropertyDataReply process_get_property_data(const GetPropertyData& msg);
    SetPropertyDataReply process_set_property_data(const SetPropertyData& msg);
    SubscribePropertyReply process_subscribe_property(const SubscribeProperty& msg);
    
    // Subscription management
    std::vector<PropertySubscription> get_subscriptions() const;
    void shutdownSubscription(uint32_t subscriber_muid, const std::string& property_id, const std::string& res_id);
    void terminateSubscriptionsToAllSubscribers(uint8_t group);
    SubscribeProperty createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, const std::string& res_id, uint8_t group, uint8_t request_id);
    
    // Notification callbacks
    void set_property_updated_callback(PropertyUpdatedCallback callback);
    void set_subscription_changed_callback(SubscriptionChangedCallback callback);
    void notify_property_updated(const std::string& property_id);
    void notify_subscription_changed(const std::string& property_id);
    
    // Legacy compatibility methods
    void remove_property(const std::string& property_id);
    void update_property(const std::string& property_id, const std::vector<uint8_t>& data);
    void update_property_metadata(const std::string& property_id, std::unique_ptr<PropertyMetadata> new_metadata);
    const PropertyMetadata* get_property_metadata(const std::string& property_id) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
