#include "midicci/midicci.hpp"
#include <algorithm>
#include <mutex>

namespace midicci {

class ObservableProfileList::Impl {
public:
    std::vector<MidiCIProfile> profiles_;
    std::vector<ProfilesChangedCallback> profiles_changed_callbacks_;
    std::vector<ProfileEnabledChangedCallback> profile_enabled_changed_callbacks_;
    std::vector<ProfileUpdatedCallback> profile_updated_callbacks_;
    mutable std::recursive_mutex mutex_;
};

ObservableProfileList::ObservableProfileList() : pimpl_(std::make_unique<Impl>()) {}

ObservableProfileList::~ObservableProfileList() = default;

const std::vector<MidiCIProfile>& ObservableProfileList::getProfiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->profiles_;
}

void ObservableProfileList::add(const MidiCIProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&profile](const MidiCIProfile& p) {
            return p.profile == profile.profile && p.address == profile.address;
        });
    
    if (it != pimpl_->profiles_.end()) {
        return;
    }
    
    pimpl_->profiles_.push_back(profile);
    
    for (const auto& callback : pimpl_->profiles_changed_callbacks_) {
        callback(ProfilesChange::Added, profile);
    }
}

void ObservableProfileList::remove(const MidiCIProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::remove_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&profile](const MidiCIProfile& p) {
            return p.profile == profile.profile && p.group == profile.group && p.address == profile.address;
        });
    
    for (auto removed_it = it; removed_it != pimpl_->profiles_.end(); ++removed_it) {
        for (const auto& callback : pimpl_->profiles_changed_callbacks_) {
            callback(ProfilesChange::Removed, *removed_it);
        }
    }
    
    pimpl_->profiles_.erase(it, pimpl_->profiles_.end());
}

void ObservableProfileList::setEnabled(bool enabled, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [address, &profile_id](const MidiCIProfile& p) {
            return p.address == address && p.profile.toString() == profile_id.toString();
        });
    
    if (it != pimpl_->profiles_.end()) {
        it->enabled = enabled;
        it->num_channels_requested = num_channels_requested;
        
        for (const auto& callback : pimpl_->profile_enabled_changed_callbacks_) {
            callback(*it);
        }
    }
}

void ObservableProfileList::update(MidiCIProfile& profile, bool enabled, uint8_t address, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    uint8_t old_address = profile.address;
    profile.enabled = enabled;
    profile.address = address;
    profile.num_channels_requested = num_channels_requested;
    
    for (const auto& callback : pimpl_->profile_updated_callbacks_) {
        callback(profile.profile, old_address, enabled, address, num_channels_requested);
    }
}

std::vector<MidiCIProfileId> ObservableProfileList::getMatchingProfiles(uint8_t address, bool enabled) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    std::vector<MidiCIProfileId> result;
    for (const auto& profile : pimpl_->profiles_) {
        if (profile.address == address && profile.enabled == enabled) {
            result.push_back(profile.profile);
        }
    }
    return result;
}

void ObservableProfileList::addProfilesChangedCallback(ProfilesChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_changed_callbacks_.push_back(callback);
}

void ObservableProfileList::addProfileEnabledChangedCallback(ProfileEnabledChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profile_enabled_changed_callbacks_.push_back(callback);
}

void ObservableProfileList::addProfileUpdatedCallback(ProfileUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profile_updated_callbacks_.push_back(callback);
}

} // namespace
