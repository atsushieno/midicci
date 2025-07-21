#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include "MidiCIProfile.hpp"

namespace midicci {

class ObservableProfileList {
public:
    enum class ProfilesChange { Added, Removed };
    
    using ProfilesChangedCallback = std::function<void(ProfilesChange, const MidiCIProfile&)>;
    using ProfileEnabledChangedCallback = std::function<void(const MidiCIProfile&)>;
    using ProfileUpdatedCallback = std::function<void(const MidiCIProfileId&, uint8_t oldAddress, bool newEnabled, uint8_t newAddress, uint16_t numChannelsRequested)>;
    
    ObservableProfileList();
    ~ObservableProfileList();
    
    ObservableProfileList(const ObservableProfileList&) = delete;
    ObservableProfileList& operator=(const ObservableProfileList&) = delete;
    
    ObservableProfileList(ObservableProfileList&&) = default;
    ObservableProfileList& operator=(ObservableProfileList&&) = default;
    
    const std::vector<MidiCIProfile>& get_profiles() const;
    
    void add(const MidiCIProfile& profile);
    void remove(const MidiCIProfile& profile);
    
    void set_enabled(bool enabled, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels_requested);
    void update(MidiCIProfile& profile, bool enabled, uint8_t address, uint16_t num_channels_requested);
    
    std::vector<MidiCIProfileId> get_matching_profiles(uint8_t address, bool enabled) const;
    
    void add_profiles_changed_callback(ProfilesChangedCallback callback);
    void add_profile_enabled_changed_callback(ProfileEnabledChangedCallback callback);
    void add_profile_updated_callback(ProfileUpdatedCallback callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
