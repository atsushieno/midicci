#pragma once

#include <memory>
#include <cstdint>

namespace midi_ci {
namespace profiles {
struct ProfileId;
}
}

namespace ci_tool {

class MidiCIProfileState {
public:
    explicit MidiCIProfileState(uint8_t grp, uint8_t addr, 
                               const midi_ci::profiles::ProfileId& prof,
                               bool en, uint16_t channels);
    ~MidiCIProfileState();
    
    MidiCIProfileState(const MidiCIProfileState&) = delete;
    MidiCIProfileState& operator=(const MidiCIProfileState&) = delete;
    
    MidiCIProfileState(MidiCIProfileState&&) = default;
    MidiCIProfileState& operator=(MidiCIProfileState&&) = default;
    
    uint8_t get_group() const noexcept;
    void set_group(uint8_t group) noexcept;
    
    uint8_t get_address() const noexcept;
    void set_address(uint8_t address) noexcept;
    
    const midi_ci::profiles::ProfileId& get_profile() const noexcept;
    
    bool is_enabled() const noexcept;
    void set_enabled(bool enabled) noexcept;
    
    uint16_t get_num_channels_requested() const noexcept;
    void set_num_channels_requested(uint16_t channels) noexcept;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
