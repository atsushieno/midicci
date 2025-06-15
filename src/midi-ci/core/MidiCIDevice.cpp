#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include <unordered_map>
#include <mutex>

namespace midi_ci {
namespace core {

class MidiCIDevice::Impl {
public:
    Impl() : device_id_(0x7F), initialized_(false) {}
    
    uint8_t device_id_;
    bool initialized_;
    MessageCallback message_callback_;
    std::unordered_map<uint8_t, std::shared_ptr<ClientConnection>> connections_;
    mutable std::mutex mutex_;
};

MidiCIDevice::MidiCIDevice() : pimpl_(std::make_unique<Impl>()) {}

MidiCIDevice::~MidiCIDevice() = default;

void MidiCIDevice::initialize() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (!pimpl_->initialized_) {
        pimpl_->initialized_ = true;
    }
}

void MidiCIDevice::shutdown() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_) {
        pimpl_->connections_.clear();
        pimpl_->initialized_ = false;
    }
}

bool MidiCIDevice::is_initialized() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->initialized_;
}

void MidiCIDevice::set_device_id(uint8_t device_id) noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->device_id_ = device_id;
}

uint8_t MidiCIDevice::get_device_id() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->device_id_;
}

void MidiCIDevice::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->message_callback_ = std::move(callback);
}

std::shared_ptr<ClientConnection> MidiCIDevice::create_connection(uint8_t destination_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    auto connection = std::make_shared<ClientConnection>(destination_id);
    pimpl_->connections_[destination_id] = connection;
    return connection;
}

void MidiCIDevice::remove_connection(uint8_t destination_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->connections_.erase(destination_id);
}

void MidiCIDevice::process_incoming_message(const std::vector<uint8_t>& message_data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->message_callback_ && !message_data.empty()) {
    }
}

} // namespace core
} // namespace midi_ci
