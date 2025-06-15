#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>

namespace midi_ci {
namespace profiles {

struct ProfileId {
    std::vector<uint8_t> bytes;
    
    explicit ProfileId(const std::vector<uint8_t>& id_bytes);
    bool operator==(const ProfileId& other) const;
    std::string to_string() const;
};

struct Profile {
    ProfileId profile_id;
    uint8_t group;
    uint8_t address;
    bool enabled;
    uint16_t num_channels_requested;
    
    Profile(const ProfileId& id, uint8_t grp, uint8_t addr, bool en, uint16_t channels);
    bool operator==(const Profile& other) const;
};

struct ProfileDetails {
    ProfileId profile_id;
    uint8_t target;
    std::vector<uint8_t> data;
    
    ProfileDetails(const ProfileId& id, uint8_t tgt, const std::vector<uint8_t>& details);
};

class ProfileManager {
public:
    using ProfileCallback = std::function<void(const Profile&)>;
    
    explicit ProfileManager();
    ~ProfileManager();
    
    ProfileManager(const ProfileManager&) = delete;
    ProfileManager& operator=(const ProfileManager&) = delete;
    
    ProfileManager(ProfileManager&&) = default;
    ProfileManager& operator=(ProfileManager&&) = default;
    
    void add_profile(const Profile& profile);
    void remove_profile(uint8_t group, uint8_t address, const ProfileId& profile_id);
    
    void enable_profile(uint8_t group, uint8_t address, const ProfileId& profile_id, uint16_t num_channels);
    void disable_profile(uint8_t group, uint8_t address, const ProfileId& profile_id);
    
    void update_profile_target(const ProfileId& profile_id, uint8_t old_address, uint8_t new_address,
                              bool enabled, uint16_t num_channels_requested);
    
    std::vector<Profile> get_profiles() const;
    std::vector<Profile> get_enabled_profiles() const;
    std::vector<Profile> get_disabled_profiles() const;
    
    Profile* find_profile(uint8_t group, uint8_t address, const ProfileId& profile_id);
    const Profile* find_profile(uint8_t group, uint8_t address, const ProfileId& profile_id) const;
    
    void add_profile_details(const ProfileDetails& details);
    void remove_profile_details(const ProfileId& profile_id, uint8_t target);
    std::vector<uint8_t> get_profile_details(const ProfileId& profile_id, uint8_t target) const;
    
    void set_profile_set_callback(ProfileCallback callback);
    void add_profile_change_callback(ProfileCallback callback);
    void remove_profile_change_callback(ProfileCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace profiles
} // namespace midi_ci
