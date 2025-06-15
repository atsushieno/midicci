#include "midi-ci/profiles/ProfileManager.hpp"
#include <mutex>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace midi_ci {
namespace profiles {

ProfileId::ProfileId(const std::vector<uint8_t>& id_bytes) : bytes(id_bytes) {}

bool ProfileId::operator==(const ProfileId& other) const {
    return bytes == other.bytes;
}

std::string ProfileId::to_string() const {
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

Profile::Profile(const ProfileId& id, uint8_t grp, uint8_t addr, bool en, uint16_t channels)
    : profile_id(id), group(grp), address(addr), enabled(en), num_channels_requested(channels) {}

bool Profile::operator==(const Profile& other) const {
    return profile_id == other.profile_id && group == other.group && address == other.address;
}

ProfileDetails::ProfileDetails(const ProfileId& id, uint8_t tgt, const std::vector<uint8_t>& details)
    : profile_id(id), target(tgt), data(details) {}

class ProfileManager::Impl {
public:
    std::vector<Profile> profiles_;
    std::vector<ProfileDetails> profile_details_;
    std::vector<ProfileCallback> change_callbacks_;
    ProfileCallback set_callback_;
    mutable std::recursive_mutex mutex_;
};

ProfileManager::ProfileManager() : pimpl_(std::make_unique<Impl>()) {}

ProfileManager::~ProfileManager() = default;

void ProfileManager::add_profile(const Profile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find(pimpl_->profiles_.begin(), pimpl_->profiles_.end(), profile);
    if (it == pimpl_->profiles_.end()) {
        pimpl_->profiles_.push_back(profile);
        
        for (const auto& callback : pimpl_->change_callbacks_) {
            callback(profile);
        }
    }
}

void ProfileManager::remove_profile(uint8_t group, uint8_t address, const ProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.group == group && p.address == address && p.profile_id == profile_id;
        });
    
    if (it != pimpl_->profiles_.end()) {
        Profile removed_profile = *it;
        pimpl_->profiles_.erase(it);
        
        for (const auto& callback : pimpl_->change_callbacks_) {
            callback(removed_profile);
        }
    }
}

void ProfileManager::enable_profile(uint8_t group, uint8_t address, const ProfileId& profile_id, uint16_t num_channels) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.group == group && p.address == address && p.profile_id == profile_id;
        });
    
    if (it != pimpl_->profiles_.end()) {
        it->enabled = true;
        it->num_channels_requested = num_channels;
        
        if (pimpl_->set_callback_) {
            pimpl_->set_callback_(*it);
        }
        
        for (const auto& callback : pimpl_->change_callbacks_) {
            callback(*it);
        }
    }
}

void ProfileManager::disable_profile(uint8_t group, uint8_t address, const ProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.group == group && p.address == address && p.profile_id == profile_id;
        });
    
    if (it != pimpl_->profiles_.end()) {
        it->enabled = false;
        
        for (const auto& callback : pimpl_->change_callbacks_) {
            callback(*it);
        }
    }
}

void ProfileManager::update_profile_target(const ProfileId& profile_id, uint8_t old_address, uint8_t new_address,
                                          bool enabled, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.profile_id == profile_id && p.address == old_address;
        });
    
    if (it != pimpl_->profiles_.end()) {
        it->address = new_address;
        it->enabled = enabled;
        it->num_channels_requested = num_channels_requested;
        
        for (const auto& callback : pimpl_->change_callbacks_) {
            callback(*it);
        }
    }
}

std::vector<Profile> ProfileManager::get_profiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->profiles_;
}

std::vector<Profile> ProfileManager::get_enabled_profiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    std::vector<Profile> enabled_profiles;
    std::copy_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
                 std::back_inserter(enabled_profiles),
                 [](const Profile& p) { return p.enabled; });
    
    return enabled_profiles;
}

std::vector<Profile> ProfileManager::get_disabled_profiles() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    std::vector<Profile> disabled_profiles;
    std::copy_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
                 std::back_inserter(disabled_profiles),
                 [](const Profile& p) { return !p.enabled; });
    
    return disabled_profiles;
}

Profile* ProfileManager::find_profile(uint8_t group, uint8_t address, const ProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.group == group && p.address == address && p.profile_id == profile_id;
        });
    
    return (it != pimpl_->profiles_.end()) ? &(*it) : nullptr;
}

const Profile* ProfileManager::find_profile(uint8_t group, uint8_t address, const ProfileId& profile_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [&](const Profile& p) {
            return p.group == group && p.address == address && p.profile_id == profile_id;
        });
    
    return (it != pimpl_->profiles_.end()) ? &(*it) : nullptr;
}

void ProfileManager::add_profile_details(const ProfileDetails& details) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profile_details_.begin(), pimpl_->profile_details_.end(),
        [&](const ProfileDetails& pd) {
            return pd.profile_id == details.profile_id && pd.target == details.target;
        });
    
    if (it != pimpl_->profile_details_.end()) {
        it->data = details.data;
    } else {
        pimpl_->profile_details_.push_back(details);
    }
}

void ProfileManager::remove_profile_details(const ProfileId& profile_id, uint8_t target) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profile_details_.begin(), pimpl_->profile_details_.end(),
        [&](const ProfileDetails& pd) {
            return pd.profile_id == profile_id && pd.target == target;
        });
    
    if (it != pimpl_->profile_details_.end()) {
        pimpl_->profile_details_.erase(it);
    }
}

std::vector<uint8_t> ProfileManager::get_profile_details(const ProfileId& profile_id, uint8_t target) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profile_details_.begin(), pimpl_->profile_details_.end(),
        [&](const ProfileDetails& pd) {
            return pd.profile_id == profile_id && pd.target == target;
        });
    
    return (it != pimpl_->profile_details_.end()) ? it->data : std::vector<uint8_t>();
}

void ProfileManager::set_profile_set_callback(ProfileCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->set_callback_ = callback;
}

void ProfileManager::add_profile_change_callback(ProfileCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->change_callbacks_.push_back(callback);
}

void ProfileManager::remove_profile_change_callback(ProfileCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->change_callbacks_.begin(), pimpl_->change_callbacks_.end(),
        [&callback](const ProfileCallback& cb) {
            return cb.target_type() == callback.target_type();
        });
    if (it != pimpl_->change_callbacks_.end()) {
        pimpl_->change_callbacks_.erase(it);
    }
}

} // namespace profiles
} // namespace midi_ci
