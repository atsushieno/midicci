#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "../messages/Message.hpp"

namespace midicci {
namespace core {
class MidiCIDevice;
}

namespace properties {
class MidiCIServicePropertyRules;
struct PropertyMetadata;

struct PropertySubscription {
    uint32_t subscriber_muid;
    std::string property_id;
    std::string subscription_id;
};

class PropertyHostFacade {
public:
    using PropertyUpdatedCallback = std::function<void(const std::string&)>;
    
    explicit PropertyHostFacade(core::MidiCIDevice& device);
    ~PropertyHostFacade();
    
    PropertyHostFacade(const PropertyHostFacade&) = delete;
    PropertyHostFacade& operator=(const PropertyHostFacade&) = delete;
    
    PropertyHostFacade(PropertyHostFacade&&) = default;
    PropertyHostFacade& operator=(PropertyHostFacade&&) = default;
    
    void set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules);
    MidiCIServicePropertyRules* get_property_rules();
    
    void add_property(std::unique_ptr<PropertyMetadata> property);
    void remove_property(const std::string& property_id);
    void update_property(const std::string& property_id, const std::vector<uint8_t>& data);
    
    messages::GetPropertyDataReply process_get_property_data(const messages::GetPropertyData& msg);
    messages::SetPropertyDataReply process_set_property_data(const messages::SetPropertyData& msg);
    messages::SubscribePropertyReply process_subscribe_property(const messages::SubscribeProperty& msg);
    
    void notify_property_updated(const std::string& property_id);
    
    void set_property_updated_callback(PropertyUpdatedCallback callback);
    
    // Additional methods for test support
    void setPropertyValue(const std::string& property_id, const std::string& res_id, const std::vector<uint8_t>& data, bool notify);
    std::vector<uint8_t> getProperty(const std::string& property_id) const;
    std::vector<PropertySubscription> get_subscriptions() const;
    void shutdownSubscription(uint32_t subscriber_muid, const std::string& property_id);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace properties
} // namespace midi_ci
