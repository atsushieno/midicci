#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <string>

namespace midi_ci {
namespace core {
class ClientConnection;
}
namespace profiles {
struct ProfileId;
}
namespace properties {
struct PropertyMetadata;
}
}

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
};

class ClientConnectionModel {
public:
    explicit ClientConnectionModel(std::shared_ptr<CIDeviceModel> parent,
                                 std::shared_ptr<midi_ci::core::ClientConnection> connection);
    ~ClientConnectionModel();
    
    ClientConnectionModel(const ClientConnectionModel&) = delete;
    ClientConnectionModel& operator=(const ClientConnectionModel&) = delete;
    
    std::shared_ptr<midi_ci::core::ClientConnection> get_connection() const;
    
    std::vector<std::shared_ptr<MidiCIProfileState>> get_profiles() const;
    
    void set_profile(uint8_t group, uint8_t address, const midi_ci::profiles::ProfileId& profile,
                    bool new_enabled, uint16_t new_num_channels_requested);
    
    std::vector<midi_ci::properties::PropertyMetadata> get_metadata_list() const;
    std::vector<SubscriptionState> get_subscriptions() const;
    
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
    
private:
    void setup_profile_listeners();
    void setup_property_listeners();
    void on_profile_changed();
    void on_property_value_updated();
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
