#pragma once

#include <memory>
#include <cstdint>
#include "../messages/Message.hpp"
#include "ObservableProfileList.hpp"
#include "../core/MidiCIDevice.hpp"

namespace midicci {
namespace profilecommonrules {

class ProfileClientFacade {
public:
    ProfileClientFacade(core::MidiCIDevice& device, core::ClientConnection& conn);
    ~ProfileClientFacade();
    
    ProfileClientFacade(const ProfileClientFacade&) = delete;
    ProfileClientFacade& operator=(const ProfileClientFacade&) = delete;
    
    ProfileClientFacade(ProfileClientFacade&&) = default;
    ProfileClientFacade& operator=(ProfileClientFacade&&) = default;
    
    ObservableProfileList& get_profiles();
    const ObservableProfileList& get_profiles() const;
    
    void set_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile, bool enabled, uint16_t num_channels_requested);
    
    void process_profile_reply(const ProfileReply& msg);
    void process_profile_added_report(const ProfileAdded& msg);
    void process_profile_removed_report(const ProfileRemoved& msg);
    void process_profile_enabled_report(const ProfileEnabled& msg);
    void process_profile_disabled_report(const ProfileDisabled& msg);
    void process_profile_details_reply(const ProfileDetailsReply& msg);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace profiles
} // namespace midi_ci
