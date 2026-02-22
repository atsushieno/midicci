#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <string>
#include "MidiCIProfileState.hpp"
#include "ClientConnectionModel.hpp"
#include "MutableState.hpp"
#include <midicci/details/musicdevice/MidiCISession.hpp>

namespace midicci::tooling {

class CIDeviceManager;

class CIDeviceModel : public std::enable_shared_from_this<CIDeviceModel> {
public:
    using ConnectionsChangedCallback = std::function<void()>;
    using ProfilesUpdatedCallback = std::function<void()>;
    using PropertiesUpdatedCallback = std::function<void()>;
    
    explicit CIDeviceModel(CIDeviceManager& parent, MidiCIDeviceConfiguration& config,
                           uint32_t muid,
                           std::shared_ptr<midicci::musicdevice::MidiCISession> session,
                           MidiCIDevice::LoggerFunction logger = {});
    ~CIDeviceModel();
    
    CIDeviceModel(const CIDeviceModel&) = delete;
    CIDeviceModel& operator=(const CIDeviceModel&) = delete;
    
    void initialize();
    void shutdown();
    
private:
    void setup_event_listeners();
    void on_connections_changed();

public:
    
    std::shared_ptr<MidiCIDevice> getDevice() const;
    
    void processCiMessage(uint8_t group, const std::vector<uint8_t>& data);
    
    const MutableStateList<std::shared_ptr<ClientConnectionModel>>& getConnections() const;
    MutableStateList<std::shared_ptr<MidiCIProfileState>>& get_local_profile_states() const;
    
    void send_discovery();
    void send_profile_details_inquiry(uint8_t address, uint32_t muid,
                                      const MidiCIProfileId& profile, uint8_t target);
    
    void update_local_profile_target(const std::shared_ptr<MidiCIProfileState>& profile_state,
                                   uint8_t new_address, bool enabled, uint16_t num_channels_requested);
    void add_local_profile(const MidiCIProfile& profile);
    void remove_local_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id);

    const midicci::commonproperties::PropertyMetadata* create_new_property();
    void remove_local_property(const std::string& property_id);
    void update_property_value(const std::string& property_id, const std::string& res_id, 
                             const std::vector<uint8_t>& data);
    void updatePropertyMetadata(const std::string& property_id, const midicci::commonproperties::PropertyMetadata& metadata);
    const midicci::commonproperties::PropertyMetadata* get_local_property_metadata(const std::string& property_id) const;
    
    void add_test_profile_items();
    
    void add_connections_changed_callback(ConnectionsChangedCallback callback);
    void add_profiles_updated_callback(ProfilesUpdatedCallback callback);
    void add_properties_updated_callback(PropertiesUpdatedCallback callback);
    
    void remove_connections_changed_callback(const ConnectionsChangedCallback& callback);
    void remove_profiles_updated_callback(const ProfilesUpdatedCallback& callback);
    void remove_properties_updated_callback(const PropertiesUpdatedCallback& callback);
    
    bool receiving_midi_message_reports;
    uint8_t last_chunked_message_channel;
    std::vector<uint8_t> chunked_messages;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
