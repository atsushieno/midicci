#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
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
    SysExSender sysex_sender_;
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

void ClientConnection::set_sysex_sender(SysExSender sender) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->sysex_sender_ = std::move(sender);
}

void ClientConnection::send_message(const midi_ci::messages::Message& message) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->connected_ && pimpl_->sysex_sender_) {
        auto packets = message.serialize_multi();
        for (const auto& packet : packets) {
            std::vector<uint8_t> sysex_data;
            sysex_data.push_back(0xF0);
            sysex_data.insert(sysex_data.end(), packet.begin(), packet.end());
            sysex_data.push_back(0xF7);
            
            pimpl_->sysex_sender_(0, sysex_data);
        }
    }
}

void ClientConnection::process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->message_callback_ && sysex_data.size() >= 5) {
        if (sysex_data[0] == 0xF0 && 
            sysex_data[1] == 0x7E &&
            sysex_data[3] == 0x0D &&
            sysex_data.back() == 0xF7) {
            
            std::vector<uint8_t> midi_ci_data(sysex_data.begin() + 1, sysex_data.end() - 1);
            
            if (midi_ci_data.size() >= 6) {
                midi_ci::messages::MessageType msg_type = static_cast<midi_ci::messages::MessageType>(midi_ci_data[4]);
                
                switch (msg_type) {
                    case midi_ci::messages::MessageType::GetPropertyData: {
                        midi_ci::messages::GetPropertyData msg(midi_ci::messages::Common{0, 0, 0, 0}, 0, std::vector<uint8_t>{});
                        if (msg.deserialize(midi_ci_data)) {
                            pimpl_->message_callback_(msg);
                        }
                        break;
                    }
                    case midi_ci::messages::MessageType::SetPropertyData: {
                        midi_ci::messages::SetPropertyData msg(midi_ci::messages::Common{0, 0, 0, 0}, 0, std::vector<uint8_t>{}, std::vector<uint8_t>{});
                        if (msg.deserialize(midi_ci_data)) {
                            pimpl_->message_callback_(msg);
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
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
