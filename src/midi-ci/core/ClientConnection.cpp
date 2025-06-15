#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include <mutex>

namespace midi_ci {
namespace core {

class ClientConnection::Impl {
public:
    explicit Impl(uint8_t destination_id) 
        : destination_id_(destination_id), connected_(true) {}
    
    uint8_t destination_id_;
    bool connected_;
    MessageCallback message_callback_;
    mutable std::mutex mutex_;
};

ClientConnection::ClientConnection(uint8_t destination_id) 
    : pimpl_(std::make_unique<Impl>(destination_id)) {}

ClientConnection::~ClientConnection() = default;

uint8_t ClientConnection::get_destination_id() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->destination_id_;
}

void ClientConnection::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->message_callback_ = std::move(callback);
}

void ClientConnection::send_message(const Message& message) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->connected_) {
        auto serialized = message.serialize();
    }
}

void ClientConnection::process_incoming_message(const std::vector<uint8_t>& message_data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->message_callback_ && !message_data.empty()) {
    }
}

bool ClientConnection::is_connected() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->connected_;
}

void ClientConnection::disconnect() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->connected_ = false;
}

} // namespace core
} // namespace midi_ci
