#include "midi-ci/profiles/ProfileClientFacade.hpp"
#include "midi-ci/profiles/ObservableProfileList.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include <mutex>

namespace midi_ci {
namespace profiles {

class ProfileClientFacade::Impl {
public:
    Impl(core::MidiCIDevice& device, core::ClientConnection& conn) 
        : device_(device), conn_(conn), profiles_(std::make_unique<ObservableProfileList>()) {}
    
    core::MidiCIDevice& device_;
    core::ClientConnection& conn_;
    std::unique_ptr<ObservableProfileList> profiles_;
    mutable std::recursive_mutex mutex_;
};

ProfileClientFacade::ProfileClientFacade(core::MidiCIDevice& device, core::ClientConnection& conn) 
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

void ProfileClientFacade::process_profile_reply(const messages::ProfileReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

void ProfileClientFacade::process_profile_added_report(const messages::ProfileAdded& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

void ProfileClientFacade::process_profile_removed_report(const messages::ProfileRemoved& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

void ProfileClientFacade::process_profile_enabled_report(const messages::ProfileEnabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

void ProfileClientFacade::process_profile_disabled_report(const messages::ProfileDisabled& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

void ProfileClientFacade::process_profile_details_reply(const messages::ProfileDetailsReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
}

} // namespace profiles
} // namespace midi_ci
