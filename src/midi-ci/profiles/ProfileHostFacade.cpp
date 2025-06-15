#include "midi-ci/profiles/ProfileHostFacade.hpp"
#include "midi-ci/profiles/ObservableProfileList.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/messages/Messenger.hpp"
#include <mutex>

namespace midi_ci {
namespace profiles {

class ProfileHostFacade::Impl {
public:
    explicit Impl(core::MidiCIDevice& device) : device_(device), profiles_(std::make_unique<ObservableProfileList>()) {}
    
    core::MidiCIDevice& device_;
    std::unique_ptr<ObservableProfileList> profiles_;
    mutable std::recursive_mutex mutex_;
};

ProfileHostFacade::ProfileHostFacade(core::MidiCIDevice& device) : pimpl_(std::make_unique<Impl>(device)) {}

ProfileHostFacade::~ProfileHostFacade() = default;

ObservableProfileList& ProfileHostFacade::get_profiles() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

const ObservableProfileList& ProfileHostFacade::get_profiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

void ProfileHostFacade::add_profile(const MidiCIProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->add(profile);
}

void ProfileHostFacade::remove_profile(const MidiCIProfileId& profile_id, uint8_t group, uint8_t address) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    MidiCIProfile profile(profile_id, group, address, false, 0);
    pimpl_->profiles_->remove(profile);
}

void ProfileHostFacade::enable_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->set_enabled(true, address, profile_id, num_channels);
}

void ProfileHostFacade::disable_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->set_enabled(false, address, profile_id, num_channels);
}

} // namespace profiles
} // namespace midi_ci
