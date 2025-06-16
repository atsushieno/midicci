#include "CIDeviceModel.hpp"
#include "CIDeviceManager.hpp"
#include "ClientConnectionModel.hpp"
#include "MidiCIProfileState.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/profiles/ProfileManager.hpp"
#include "midi-ci/properties/PropertyManager.hpp"
#include <mutex>
#include <iostream>

namespace ci_tool {

class CIDeviceModel::Impl {
public:
    explicit Impl(CIDeviceManager& parent, uint32_t muid,
                  CIOutputSender ci_sender, MidiMessageReportSender mmr_sender)
        : parent_(parent), muid_(muid),
          ci_output_sender_(ci_sender), midi_message_report_sender_(mmr_sender),
          receiving_midi_message_reports_(false), last_chunked_message_channel_(0) {}
    
    CIDeviceManager& parent_;
    uint32_t muid_;
    CIOutputSender ci_output_sender_;
    MidiMessageReportSender midi_message_report_sender_;
    
    std::shared_ptr<midi_ci::core::MidiCIDevice> device_;
    std::vector<std::shared_ptr<ClientConnectionModel>> connections_;
    std::vector<std::shared_ptr<MidiCIProfileState>> local_profile_states_;
    
    bool receiving_midi_message_reports_;
    uint8_t last_chunked_message_channel_;
    std::vector<uint8_t> chunked_messages_;
    
    mutable std::recursive_mutex mutex_{};
};

CIDeviceModel::CIDeviceModel(CIDeviceManager& parent, uint32_t muid,
                           CIOutputSender ci_output_sender,
                           MidiMessageReportSender midi_message_report_sender)
    : pimpl_(std::make_unique<Impl>(parent, muid, ci_output_sender, midi_message_report_sender)) {
    receiving_midi_message_reports = pimpl_->receiving_midi_message_reports_;
    last_chunked_message_channel = pimpl_->last_chunked_message_channel_;
    chunked_messages = pimpl_->chunked_messages_;
}

CIDeviceModel::~CIDeviceModel() = default;

void CIDeviceModel::initialize() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->device_ = std::make_shared<midi_ci::core::MidiCIDevice>();
    pimpl_->device_->set_sysex_sender(pimpl_->ci_output_sender_);
    pimpl_->device_->initialize();
    
    add_test_profile_items();
    
    std::cout << "CIDeviceModel initialized with MUID: 0x" << std::hex << pimpl_->muid_ << std::dec << std::endl;
}

void CIDeviceModel::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_->shutdown();
        pimpl_->device_.reset();
    }
    pimpl_->connections_.clear();
    pimpl_->local_profile_states_.clear();
    std::cout << "CIDeviceModel shutdown" << std::endl;
}

std::shared_ptr<midi_ci::core::MidiCIDevice> CIDeviceModel::get_device() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->device_;
}

void CIDeviceModel::process_ci_message(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        std::vector<uint8_t> sysex_data;
        sysex_data.push_back(0xF0);
        sysex_data.insert(sysex_data.end(), data.begin(), data.end());
        sysex_data.push_back(0xF7);
        pimpl_->device_->process_incoming_sysex(group, sysex_data);
    }
}

std::vector<std::shared_ptr<ClientConnectionModel>> CIDeviceModel::get_connections() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->connections_;
}

std::vector<std::shared_ptr<MidiCIProfileState>> CIDeviceModel::get_local_profile_states() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->local_profile_states_;
}

void CIDeviceModel::send_discovery() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_->sendDiscovery();
        std::cout << "Sending discovery inquiry..." << std::endl;
    }
}

void CIDeviceModel::send_profile_details_inquiry(uint8_t address, uint32_t muid, 
                                                const midi_ci::profiles::ProfileId& profile, uint8_t target) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Sending profile details inquiry to MUID: 0x" << std::hex << muid << std::dec << std::endl;
}

void CIDeviceModel::update_local_profile_target(const std::shared_ptr<MidiCIProfileState>& profile_state,
                                               uint8_t new_address, bool enabled, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (profile_state) {
        profile_state->set_address(new_address);
        profile_state->set_enabled(enabled);
        profile_state->set_num_channels_requested(num_channels_requested);
    }
}

void CIDeviceModel::add_local_profile(const midi_ci::profiles::Profile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto profile_state = std::make_shared<MidiCIProfileState>(
        profile.group, profile.address, profile.profile_id, 
        profile.enabled, profile.num_channels_requested);
    pimpl_->local_profile_states_.push_back(profile_state);
}

void CIDeviceModel::remove_local_profile(uint8_t group, uint8_t address, const midi_ci::profiles::ProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = std::remove_if(pimpl_->local_profile_states_.begin(), pimpl_->local_profile_states_.end(),
        [group, address, &profile_id](const std::shared_ptr<MidiCIProfileState>& state) {
            return state->get_group() == group && 
                   state->get_address() == address && 
                   state->get_profile() == profile_id;
        });
    pimpl_->local_profile_states_.erase(it, pimpl_->local_profile_states_.end());
}

void CIDeviceModel::add_local_property(const midi_ci::properties::PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Added local property: " << property.property_id << std::endl;
}

void CIDeviceModel::remove_local_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Removed local property: " << property_id << std::endl;
}

void CIDeviceModel::update_property_value(const std::string& property_id, const std::string& res_id, 
                                         const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Updated property: " << property_id << " (resource: " << res_id << ")" << std::endl;
}

void CIDeviceModel::add_test_profile_items() {
    midi_ci::profiles::ProfileId test_profile({0x7E, 0x00, 0x01, 0x02, 0x03});
    midi_ci::profiles::Profile profile(test_profile, 0, 0x7F, false, 1);
    add_local_profile(profile);
    
    std::cout << "Added test profile items" << std::endl;
}

} // namespace ci_tool
