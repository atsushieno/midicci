#pragma once

#include <memory>
#include <cstdint>
#include <functional>
#include <midi-ci/profiles/MidiCIProfile.hpp>
#include "MutableState.hpp"

namespace ci_tool {

class MidiCIProfileState {
public:
    using StateChangedCallback = std::function<void()>;
    
    explicit MidiCIProfileState(uint8_t grp, uint8_t addr, 
                               const midi_ci::profiles::MidiCIProfileId& prof,
                               bool en, uint16_t channels);
    ~MidiCIProfileState();
    
    MidiCIProfileState(const MidiCIProfileState&) = delete;
    MidiCIProfileState& operator=(const MidiCIProfileState&) = delete;
    
    MidiCIProfileState(MidiCIProfileState&&) = default;
    MidiCIProfileState& operator=(MidiCIProfileState&&) = default;
    
    MutableState<uint8_t>& group();
    const MutableState<uint8_t>& group() const;
    
    MutableState<uint8_t>& address();
    const MutableState<uint8_t>& address() const;
    
    const midi_ci::profiles::MidiCIProfileId& get_profile() const noexcept;
    
    MutableState<bool>& enabled();
    const MutableState<bool>& enabled() const;
    
    MutableState<uint16_t>& num_channels_requested();
    const MutableState<uint16_t>& num_channels_requested() const;
    
    void add_state_changed_callback(StateChangedCallback callback);
    void remove_state_changed_callback(const StateChangedCallback& callback);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
