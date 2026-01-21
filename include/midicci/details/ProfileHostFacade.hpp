#pragma once

#include <memory>
#include <functional>
#include <vector>
#include "midicci/midicci.hpp"

namespace midicci {

class ProfileHostFacade {
public:
    explicit ProfileHostFacade(MidiCIDevice& device);
    ~ProfileHostFacade();
    
    ProfileHostFacade(const ProfileHostFacade&) = delete;
    ProfileHostFacade& operator=(const ProfileHostFacade&) = delete;
    
    ProfileHostFacade(ProfileHostFacade&&) = default;
    ProfileHostFacade& operator=(ProfileHostFacade&&) = default;
    
    ObservableProfileList& getProfiles();
    const ObservableProfileList& getProfiles() const;
    
    void addProfile(const MidiCIProfile& profile);
    void removeProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id);
    void enableProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels);
    void disableProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> getProfileDetails(const MidiCIProfileId& profile, uint8_t target) const;
    void updateProfileTarget(const MidiCIProfileId& profile_id, uint8_t old_address, uint8_t new_address, bool enabled, uint16_t num_channels_requested);
    std::vector<MidiCIProfileDetails>& getProfileDetailsEntries();
    const std::vector<MidiCIProfileDetails>& getProfileDetailsEntries() const;
    
    using ProfileSetCallback = std::function<void(const MidiCIProfile&)>;
    void addOnProfileSetCallback(ProfileSetCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
