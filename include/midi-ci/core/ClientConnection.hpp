#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>

namespace midi_ci {
namespace messages {
class Message;
}

namespace profiles {
class ProfileClientFacade;
}

namespace properties {
class PropertyClientFacade;
}

namespace core {

class MidiCIDevice;

class ClientConnection {
public:
    using MessageCallback = std::function<void(const messages::Message&)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit ClientConnection(MidiCIDevice& device, uint8_t destination_id);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint8_t get_destination_id() const noexcept;
    
    void set_message_callback(MessageCallback callback);
    void set_ci_output_sender(CIOutputSender sender);
    
    void send_message(const messages::Message& message);
    void process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    bool is_connected() const noexcept;
    void disconnect();
    
    profiles::ProfileClientFacade& get_profile_client_facade();
    const profiles::ProfileClientFacade& get_profile_client_facade() const;
    
    properties::PropertyClientFacade& get_property_client_facade();
    const properties::PropertyClientFacade& get_property_client_facade() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
