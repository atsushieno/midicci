#include "MidiCIProfileState.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace ci_tool {

class MidiCIProfileState::Impl {
public:
    explicit Impl(uint8_t grp, uint8_t addr, const midicci::profiles::MidiCIProfileId& prof,
                  bool en, uint16_t channels)
        : group_(grp), address_(addr), profile_(prof), enabled_(en), num_channels_requested_(channels) {
        
        group_.set_value_changed_handler([this](const uint8_t& newValue) {
            notify_state_changed();
        });
        
        address_.set_value_changed_handler([this](const uint8_t& newValue) {
            notify_state_changed();
        });
        
        enabled_.set_value_changed_handler([this](const bool& newValue) {
            notify_state_changed();
        });
        
        num_channels_requested_.set_value_changed_handler([this](const uint16_t& newValue) {
            notify_state_changed();
        });
    }
    
    void notify_state_changed() {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : state_changed_callbacks_) {
            callback();
        }
    }
    
    MutableState<uint8_t> group_;
    MutableState<uint8_t> address_;
    midicci::profiles::MidiCIProfileId profile_;
    MutableState<bool> enabled_;
    MutableState<uint16_t> num_channels_requested_;
    
    std::vector<StateChangedCallback> state_changed_callbacks_;
    mutable std::mutex mutex_;
};

MidiCIProfileState::MidiCIProfileState(uint8_t grp, uint8_t addr, 
                                     const midicci::profiles::MidiCIProfileId& prof,
                                     bool en, uint16_t channels)
    : pimpl_(std::make_unique<Impl>(grp, addr, prof, en, channels)) {}

MidiCIProfileState::~MidiCIProfileState() = default;

MutableState<uint8_t>& MidiCIProfileState::group() {
    return pimpl_->group_;
}

const MutableState<uint8_t>& MidiCIProfileState::group() const {
    return pimpl_->group_;
}

MutableState<uint8_t>& MidiCIProfileState::address() {
    return pimpl_->address_;
}

const MutableState<uint8_t>& MidiCIProfileState::address() const {
    return pimpl_->address_;
}

const midicci::profiles::MidiCIProfileId& MidiCIProfileState::get_profile() const noexcept {
    return pimpl_->profile_;
}

MutableState<bool>& MidiCIProfileState::enabled() {
    return pimpl_->enabled_;
}

const MutableState<bool>& MidiCIProfileState::enabled() const {
    return pimpl_->enabled_;
}

MutableState<uint16_t>& MidiCIProfileState::num_channels_requested() {
    return pimpl_->num_channels_requested_;
}

const MutableState<uint16_t>& MidiCIProfileState::num_channels_requested() const {
    return pimpl_->num_channels_requested_;
}

void MidiCIProfileState::add_state_changed_callback(StateChangedCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->state_changed_callbacks_.push_back(callback);
}

void MidiCIProfileState::remove_state_changed_callback(const StateChangedCallback& callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->state_changed_callbacks_.erase(
        std::remove_if(pimpl_->state_changed_callbacks_.begin(), 
                      pimpl_->state_changed_callbacks_.end(),
                      [&callback](const StateChangedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->state_changed_callbacks_.end());
}

} // namespace ci_tool
