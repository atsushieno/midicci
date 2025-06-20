#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <string>
#include "midicci/core/ClientConnection.hpp"
#include "midicci/properties/ObservablePropertyList.hpp"
#include "MutableState.hpp"

using namespace midicci;

namespace ci_tool {

class CIDeviceModel;
class MidiCIProfileState;

struct SubscriptionState {
    std::string property_id;
    enum class State {
        Subscribing,
        Subscribed,
        Unsubscribed
    } state;
    
    SubscriptionState(const std::string& id, State s) : property_id(id), state(s) {}
    
    bool operator==(const SubscriptionState& other) const {
        return property_id == other.property_id && state == other.state;
    }
};

class ClientConnectionModel {
public:
    using ProfilesChangedCallback = std::function<void()>;
    using PropertiesChangedCallback = std::function<void()>;
    using DeviceInfoChangedCallback = std::function<void()>;
    
    explicit ClientConnectionModel(std::shared_ptr<CIDeviceModel> parent,
                                 std::shared_ptr<ClientConnection> connection);
    ~ClientConnectionModel();
    
    ClientConnectionModel(const ClientConnectionModel&) = delete;
    ClientConnectionModel& operator=(const ClientConnectionModel&) = delete;
    
    std::shared_ptr<ClientConnection> get_connection() const;
    const MutableStateList<std::shared_ptr<MidiCIProfileState>>& get_profiles() const;
    const MutableStateList<SubscriptionState>& get_subscriptions() const;
    const MutableStateList<midicci::propertycommonrules::PropertyValue>& get_properties() const;
    
    std::string get_device_info_value() const;
    const MutableState<std::string>& get_device_info() const;
    
    void set_profile(uint8_t group, uint8_t address, const midicci::profilecommonrules::MidiCIProfileId& profile,
                    bool new_enabled, uint16_t new_num_channels_requested);
    
    std::vector<std::unique_ptr<midicci::propertycommonrules::PropertyMetadata>> get_metadata_list() const;
    
    void get_property_data(const std::string& resource, const std::string& encoding = "",
                          int paginate_offset = -1, int paginate_limit = -1);
    void set_property_data(const std::string& resource, const std::string& res_id,
                          const std::vector<uint8_t>& data, const std::string& encoding = "",
                          bool is_partial = false);
    void subscribe_property(const std::string& resource, const std::string& mutual_encoding = "");
    void unsubscribe_property(const std::string& resource);
    
    void request_midi_message_report(uint8_t address, uint32_t target_muid,
                                   uint8_t message_data_control = 0xFF,
                                   uint8_t system_messages = 0xFF,
                                   uint8_t channel_controller_messages = 0xFF,
                                   uint8_t note_data_messages = 0xFF);
    
    void add_profiles_changed_callback(ProfilesChangedCallback callback);
    void add_properties_changed_callback(PropertiesChangedCallback callback);
    void add_device_info_changed_callback(DeviceInfoChangedCallback callback);
    
    void remove_profiles_changed_callback(const ProfilesChangedCallback& callback);
    void remove_properties_changed_callback(const PropertiesChangedCallback& callback);
    void remove_device_info_changed_callback(const DeviceInfoChangedCallback& callback);
    
private:
    void setup_profile_listeners();
    void setup_property_listeners();
    void on_profile_changed();
    void on_property_value_updated();
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
