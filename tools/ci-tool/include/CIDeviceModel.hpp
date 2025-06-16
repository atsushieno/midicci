#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <string>
#include "MidiCIProfileState.hpp"
#include "ClientConnectionModel.hpp"

namespace midi_ci {
namespace core {
class MidiCIDevice;
}
namespace profiles {
struct Profile;
struct ProfileId;
}
namespace properties {
struct PropertyMetadata;
}
}

namespace ci_tool {

class CIDeviceManager;

class CIDeviceModel {
    
public:
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    using MidiMessageReportSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit CIDeviceModel(CIDeviceManager& parent, uint32_t muid,
                          CIOutputSender ci_output_sender,
                          MidiMessageReportSender midi_message_report_sender,
                          std::function<void(const std::string&, bool)> logger = {});
    ~CIDeviceModel();
    
    CIDeviceModel(const CIDeviceModel&) = delete;
    CIDeviceModel& operator=(const CIDeviceModel&) = delete;
    
    void initialize();
    void shutdown();
    
    std::shared_ptr<midi_ci::core::MidiCIDevice> get_device() const;
    
    void process_ci_message(uint8_t group, const std::vector<uint8_t>& data);
    
    std::vector<std::shared_ptr<ClientConnectionModel>> get_connections() const;
    std::vector<std::shared_ptr<MidiCIProfileState>> get_local_profile_states() const;
    
    void send_discovery();
    void send_profile_details_inquiry(uint8_t address, uint32_t muid, 
                                    const midi_ci::profiles::ProfileId& profile, uint8_t target);
    
    void update_local_profile_target(const std::shared_ptr<MidiCIProfileState>& profile_state,
                                   uint8_t new_address, bool enabled, uint16_t num_channels_requested);
    void add_local_profile(const midi_ci::profiles::Profile& profile);
    void remove_local_profile(uint8_t group, uint8_t address, const midi_ci::profiles::ProfileId& profile_id);
    
    void add_local_property(const midi_ci::properties::PropertyMetadata& property);
    void remove_local_property(const std::string& property_id);
    void update_property_value(const std::string& property_id, const std::string& res_id, 
                             const std::vector<uint8_t>& data);
    
    void add_test_profile_items();
    
    bool receiving_midi_message_reports;
    uint8_t last_chunked_message_channel;
    std::vector<uint8_t> chunked_messages;


    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
