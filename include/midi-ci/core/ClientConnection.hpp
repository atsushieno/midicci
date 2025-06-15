#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace midi_ci {
namespace messages {
class Message;
}

namespace core {

class ClientConnection {
public:
    using MessageCallback = std::function<void(const messages::Message&)>;
    using SysExSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit ClientConnection(uint8_t destination_id);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint8_t get_destination_id() const noexcept;
    
    void set_message_callback(MessageCallback callback);
    void set_sysex_sender(SysExSender sender);
    
    void send_message(const messages::Message& message);
    void process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    bool is_connected() const noexcept;
    void disconnect();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
