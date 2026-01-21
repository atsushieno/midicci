#include "midicci/midicci.hpp"
#include <mutex>

namespace midicci {

class ProfileClientFacade::Impl {
public:
    Impl(MidiCIDevice& device, ClientConnection& conn) 
        : device_(device), conn_(conn), profiles_(std::make_unique<ObservableProfileList>()) {}
    
    MidiCIDevice& device_;
    ClientConnection& conn_;
    std::unique_ptr<ObservableProfileList> profiles_;
    mutable std::recursive_mutex mutex_;
};

ProfileClientFacade::ProfileClientFacade(MidiCIDevice& device, ClientConnection& conn) 
    : pimpl_(std::make_unique<Impl>(device, conn)) {}

ProfileClientFacade::~ProfileClientFacade() = default;

ObservableProfileList& ProfileClientFacade::getProfiles() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

const ObservableProfileList& ProfileClientFacade::getProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

void ProfileClientFacade::setProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile, bool enabled, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (enabled) {
        uint16_t channels = num_channels_requested;
        if (address < 0x10) {
            if (channels < 1) channels = 1;
        } else if (address >= 0x7E) {
            channels = 0;
        }
        
        MidiCIProfile midi_profile(profile, group, address, true, channels);
        pimpl_->profiles_->add(midi_profile);
    } else {
        MidiCIProfile midi_profile(profile, group, address, false, 0);
        pimpl_->profiles_->remove(midi_profile);
    }
}

void ProfileClientFacade::processProfileReply(const ProfileReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    for (const auto& profile_data : msg.getEnabledProfiles()) {
        uint16_t num_channels = (msg.getCommon().address >= 0x7E) ? 0 : 1;
        MidiCIProfile profile(profile_data, msg.getCommon().group, msg.getCommon().address, true, num_channels);
        pimpl_->profiles_->add(profile);
    }
    
    for (const auto& profile_data : msg.getDisabledProfiles()) {
        uint16_t num_channels = (msg.getCommon().address >= 0x7E) ? 0 : 1;
        MidiCIProfile profile(profile_data, msg.getCommon().group, msg.getCommon().address, false, num_channels);
        pimpl_->profiles_->add(profile);
    }
}

void ProfileClientFacade::processProfileAddedReport(const ProfileAdded& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.getProfileId());
    uint16_t num_channels = (msg.getCommon().address >= 0x7E) ? 0 : 1;
    MidiCIProfile profile(profile_id, msg.getCommon().group, msg.getCommon().address, false, num_channels);
    pimpl_->profiles_->add(profile);
}

void ProfileClientFacade::processProfileRemovedReport(const ProfileRemoved& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.getProfileId());
    MidiCIProfile profile(profile_id, msg.getCommon().group, msg.getCommon().address, false, 0);
    pimpl_->profiles_->remove(profile);
}

void ProfileClientFacade::processProfileEnabledReport(const ProfileEnabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.getProfileId());
    pimpl_->profiles_->setEnabled(true, msg.getCommon().address, profile_id, msg.getNumChannels());
}

void ProfileClientFacade::processProfileDisabledReport(const ProfileDisabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.getProfileId());
    pimpl_->profiles_->setEnabled(false, msg.getCommon().address, profile_id, msg.getNumChannels());
}

void ProfileClientFacade::processProfileDetailsReply(const ProfileDetailsReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

} // namespace
