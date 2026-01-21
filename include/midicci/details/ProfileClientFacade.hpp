#pragma once

#include <memory>
#include <cstdint>
#include "midicci/midicci.hpp"

namespace midicci {

class MidiCIDevice;
class ClientConnection;

class ProfileClientFacade {
public:
    ProfileClientFacade(MidiCIDevice& device, ClientConnection& conn);
    ~ProfileClientFacade();
    
    ProfileClientFacade(const ProfileClientFacade&) = delete;
    ProfileClientFacade& operator=(const ProfileClientFacade&) = delete;
    
    ProfileClientFacade(ProfileClientFacade&&) = default;
    ProfileClientFacade& operator=(ProfileClientFacade&&) = default;
    
    ObservableProfileList& getProfiles();
    const ObservableProfileList& getProfiles() const;
    
    void setProfile(uint8_t group, uint8_t address, const MidiCIProfileId& profile, bool enabled, uint16_t num_channels_requested);
    
    void processProfileReply(const ProfileReply& msg);
    void processProfileAddedReport(const ProfileAdded& msg);
    void processProfileRemovedReport(const ProfileRemoved& msg);
    void processProfileEnabledReport(const ProfileEnabled& msg);
    void processProfileDisabledReport(const ProfileDisabled& msg);
    void processProfileDetailsReply(const ProfileDetailsReply& msg);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
