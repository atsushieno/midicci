#pragma once

#include <memory>
#include <functional>

namespace midi_ci {
namespace core {
class MidiCIDevice;
}

namespace profiles {
class ObservableProfileList;
struct MidiCIProfile;
struct MidiCIProfileId;

class ProfileHostFacade {
public:
    explicit ProfileHostFacade(core::MidiCIDevice& device);
    ~ProfileHostFacade();
    
    ProfileHostFacade(const ProfileHostFacade&) = delete;
    ProfileHostFacade& operator=(const ProfileHostFacade&) = delete;
    
    ProfileHostFacade(ProfileHostFacade&&) = default;
    ProfileHostFacade& operator=(ProfileHostFacade&&) = default;
    
    ObservableProfileList& get_profiles();
    const ObservableProfileList& get_profiles() const;
    
    void add_profile(const MidiCIProfile& profile);
    void remove_profile(const MidiCIProfileId& profile_id, uint8_t group, uint8_t address);
    void enable_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels);
    void disable_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id, uint16_t num_channels);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace profiles
} // namespace midi_ci
