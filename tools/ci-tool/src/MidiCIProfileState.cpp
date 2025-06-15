#include "MidiCIProfileState.hpp"
#include "midi-ci/profiles/ProfileManager.hpp"
#include <mutex>

namespace ci_tool {

class MidiCIProfileState::Impl {
public:
    explicit Impl(uint8_t grp, uint8_t addr, const midi_ci::profiles::ProfileId& prof,
                  bool en, uint16_t channels)
        : group_(grp), address_(addr), profile_(prof), enabled_(en), num_channels_requested_(channels) {}
    
    uint8_t group_;
    uint8_t address_;
    midi_ci::profiles::ProfileId profile_;
    bool enabled_;
    uint16_t num_channels_requested_;
    mutable std::mutex mutex_;
};

MidiCIProfileState::MidiCIProfileState(uint8_t grp, uint8_t addr, 
                                     const midi_ci::profiles::ProfileId& prof,
                                     bool en, uint16_t channels)
    : pimpl_(std::make_unique<Impl>(grp, addr, prof, en, channels)) {}

MidiCIProfileState::~MidiCIProfileState() = default;

uint8_t MidiCIProfileState::get_group() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->group_;
}

void MidiCIProfileState::set_group(uint8_t group) noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->group_ = group;
}

uint8_t MidiCIProfileState::get_address() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->address_;
}

void MidiCIProfileState::set_address(uint8_t address) noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->address_ = address;
}

const midi_ci::profiles::ProfileId& MidiCIProfileState::get_profile() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->profile_;
}

bool MidiCIProfileState::is_enabled() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->enabled_;
}

void MidiCIProfileState::set_enabled(bool enabled) noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->enabled_ = enabled;
}

uint16_t MidiCIProfileState::get_num_channels_requested() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->num_channels_requested_;
}

void MidiCIProfileState::set_num_channels_requested(uint16_t channels) noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->num_channels_requested_ = channels;
}

} // namespace ci_tool
