#include "midicci/midicci.hpp"
#include <mutex>

namespace midicci {

class ProfileHostFacade::Impl {
public:
    explicit Impl(MidiCIDevice& device) : device_(device), profiles_(std::make_unique<ObservableProfileList>()) {}
    
    MidiCIDevice& device_;
    std::unique_ptr<ObservableProfileList> profiles_;
    std::vector<MidiCIProfileDetails> profile_details_entries_;
    std::vector<ProfileHostFacade::ProfileSetCallback> on_profile_set_callbacks_;
    mutable std::recursive_mutex mutex_;
};

ProfileHostFacade::ProfileHostFacade(MidiCIDevice& device) : pimpl_(std::make_unique<Impl>(device)) {}

ProfileHostFacade::~ProfileHostFacade() = default;

ObservableProfileList& ProfileHostFacade::getProfiles() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

const ObservableProfileList& ProfileHostFacade::getProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profiles_;
}

void ProfileHostFacade::addProfile(const MidiCIProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->add(profile);
}

void ProfileHostFacade::removeProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    MidiCIProfile profile(profile_id, group, address, false, 0);
    pimpl_->profiles_->remove(profile);
}

void ProfileHostFacade::enableProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->setEnabled(true, address, profile_id, num_channels);
    
    MidiCIProfile profile(profile_id, group, address, true, num_channels);
    for (const auto& callback : pimpl_->on_profile_set_callbacks_) {
        callback(profile);
    }
}

void ProfileHostFacade::disableProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_->setEnabled(false, address, profile_id, num_channels);
    
    MidiCIProfile profile(profile_id, group, address, false, num_channels);
    for (const auto& callback : pimpl_->on_profile_set_callbacks_) {
        callback(profile);
    }
}

std::vector<uint8_t> ProfileHostFacade::getProfileDetails(const MidiCIProfileId& profile, uint8_t target) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    for (const auto& entry : pimpl_->profile_details_entries_) {
        if (entry.profile == profile && entry.target == target) {
            return entry.data;
        }
    }
    return std::vector<uint8_t>();
}

void ProfileHostFacade::updateProfileTarget(const MidiCIProfileId& profile_id, uint8_t old_address, uint8_t new_address, bool enabled, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    const auto& profiles = pimpl_->profiles_->getProfiles();
    for (const auto& profile : profiles) {
        if (profile.profile == profile_id && profile.address == old_address) {
            pimpl_->profiles_->update(const_cast<MidiCIProfile&>(profile), enabled, new_address, num_channels_requested);
            break;
        }
    }
}

std::vector<MidiCIProfileDetails>& ProfileHostFacade::getProfileDetailsEntries() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->profile_details_entries_;
}

const std::vector<MidiCIProfileDetails>& ProfileHostFacade::getProfileDetailsEntries() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->profile_details_entries_;
}

void ProfileHostFacade::addOnProfileSetCallback(ProfileSetCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->on_profile_set_callbacks_.push_back(callback);
}

} // namespace
