#pragma once

#include <memory>
#include <vector>
#include <string>
#include <functional>
#include "../messages/Message.hpp"

namespace midi_ci {
namespace core {
class MidiCIDevice;
}

namespace properties {
class MidiCIServicePropertyRules;
struct PropertyMetadata;

class PropertyHostFacade {
public:
    explicit PropertyHostFacade(core::MidiCIDevice& device);
    ~PropertyHostFacade();
    
    PropertyHostFacade(const PropertyHostFacade&) = delete;
    PropertyHostFacade& operator=(const PropertyHostFacade&) = delete;
    
    PropertyHostFacade(PropertyHostFacade&&) = default;
    PropertyHostFacade& operator=(PropertyHostFacade&&) = default;
    
    void set_property_rules(std::unique_ptr<MidiCIServicePropertyRules> rules);
    MidiCIServicePropertyRules* get_property_rules();
    
    void add_property(const PropertyMetadata& property);
    void remove_property(const std::string& property_id);
    void update_property(const std::string& property_id, const std::vector<uint8_t>& data);
    
    messages::GetPropertyDataReply process_get_property_data(const messages::GetPropertyData& msg);
    messages::SetPropertyDataReply process_set_property_data(const messages::SetPropertyData& msg);
    messages::SubscribePropertyReply process_subscribe_property(const messages::SubscribeProperty& msg);
    
    void notify_property_updated(const std::string& property_id);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace properties
} // namespace midi_ci
