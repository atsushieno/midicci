#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace midi_ci {
namespace core {

class Message;

class ClientConnection {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    explicit ClientConnection(uint8_t destination_id);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint8_t get_destination_id() const noexcept;
    
    void set_message_callback(MessageCallback callback);
    
    void send_message(const Message& message);
    void process_incoming_message(const std::vector<uint8_t>& message_data);
    
    bool is_connected() const noexcept;
    void disconnect();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
