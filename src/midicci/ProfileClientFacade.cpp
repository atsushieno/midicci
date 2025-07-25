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

ObservableProfileList& ProfileClientFacade::get_profiles() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

const ObservableProfileList& ProfileClientFacade::get_profiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

void ProfileClientFacade::set_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile, bool enabled, uint16_t num_channels_requested) {
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

void ProfileClientFacade::process_profile_reply(const ProfileReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    for (const auto& profile_data : msg.get_enabled_profiles()) {
        uint16_t num_channels = (msg.get_common().address >= 0x7E) ? 0 : 1;
        MidiCIProfile profile(profile_data, msg.get_common().group, msg.get_common().address, true, num_channels);
        pimpl_->profiles_->add(profile);
    }
    
    for (const auto& profile_data : msg.get_disabled_profiles()) {
        uint16_t num_channels = (msg.get_common().address >= 0x7E) ? 0 : 1;
        MidiCIProfile profile(profile_data, msg.get_common().group, msg.get_common().address, false, num_channels);
        pimpl_->profiles_->add(profile);
    }
}

void ProfileClientFacade::process_profile_added_report(const ProfileAdded& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.get_profile_id());
    uint16_t num_channels = (msg.get_common().address >= 0x7E) ? 0 : 1;
    MidiCIProfile profile(profile_id, msg.get_common().group, msg.get_common().address, false, num_channels);
    pimpl_->profiles_->add(profile);
}

void ProfileClientFacade::process_profile_removed_report(const ProfileRemoved& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.get_profile_id());
    MidiCIProfile profile(profile_id, msg.get_common().group, msg.get_common().address, false, 0);
    pimpl_->profiles_->remove(profile);
}

void ProfileClientFacade::process_profile_enabled_report(const ProfileEnabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.get_profile_id());
    pimpl_->profiles_->set_enabled(true, msg.get_common().address, profile_id, msg.get_num_channels());
}

void ProfileClientFacade::process_profile_disabled_report(const ProfileDisabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    MidiCIProfileId profile_id(msg.get_profile_id());
    pimpl_->profiles_->set_enabled(false, msg.get_common().address, profile_id, msg.get_num_channels());
}

void ProfileClientFacade::process_profile_details_reply(const ProfileDetailsReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

} // namespace
