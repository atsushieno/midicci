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
    std::string subscription_id;
};

class PropertyHostFacade {
public:
    using PropertyUpdatedCallback = std::function<void(const std::string&)>;
    using SubscriptionChangedCallback = std::function<void(const std::string& property_id)>;
    
    explicit PropertyHostFacade(MidiCIDevice& device);
    ~PropertyHostFacade();
    
    PropertyHostFacade(const PropertyHostFacade&) = delete;
    PropertyHostFacade& operator=(const PropertyHostFacade&) = delete;
    
    PropertyHostFacade(PropertyHostFacade&&) noexcept;
    PropertyHostFacade& operator=(PropertyHostFacade&&) noexcept;
    
    void set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules);
    MidiCIServicePropertyRules* get_property_rules();
    
    void add_property(std::unique_ptr<PropertyMetadata> property);
    void remove_property(const std::string& property_id);
    void update_property(const std::string& property_id, const std::vector<uint8_t>& data);
    
    GetPropertyDataReply process_get_property_data(const GetPropertyData& msg);
    SetPropertyDataReply process_set_property_data(const SetPropertyData& msg);
    SubscribePropertyReply process_subscribe_property(const SubscribeProperty& msg);
    
    void notify_property_updated(const std::string& property_id);
    void notify_subscription_changed(const std::string& property_id);
    
    void set_property_updated_callback(PropertyUpdatedCallback callback);
    void set_subscription_changed_callback(SubscriptionChangedCallback callback);
    
    // Additional methods for test support
    void setPropertyValue(const std::string& property_id, const std::string& res_id, const std::vector<uint8_t>& data, bool notify);
    std::vector<uint8_t> getProperty(const std::string& property_id) const;
    std::vector<PropertySubscription> get_subscriptions() const;
    void shutdownSubscription(uint32_t subscriber_muid, const std::string& property_id);
    SubscribeProperty createShutdownSubscriptionMessage(uint32_t destination_muid, const std::string& property_id, uint8_t group, uint8_t request_id);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
