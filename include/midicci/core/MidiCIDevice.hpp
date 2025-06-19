#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include "midicci/messages/Messenger.hpp"

namespace midicci {
namespace profiles {
class ProfileHostFacade;
class ProfileClientFacade;
}

namespace properties {
class PropertyHostFacade;
class PropertyClientFacade;
}

namespace core {

class ClientConnection;
class Message;
struct MidiCIDeviceConfiguration;

class MidiCIDevice {
public:
    using MessageCallback = std::function<void(const Message&)>;
    using MessageReceivedCallback = std::function<void(const messages::Message&)>;
    using ConnectionsChangedCallback = std::function<void()>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    using LoggerFunction = std::function<void(const std::string&, bool)>;
    
    MidiCIDevice(uint32_t muid, MidiCIDeviceConfiguration& config, LoggerFunction logger = LoggerFunction{});
    ~MidiCIDevice();
    
    MidiCIDevice(const MidiCIDevice&) = delete;
    MidiCIDevice& operator=(const MidiCIDevice&) = delete;
    
    MidiCIDevice(MidiCIDevice&&) = default;
    MidiCIDevice& operator=(MidiCIDevice&&) = default;
    
    void initialize();
    void shutdown();
    
    bool is_initialized() const noexcept;
    
    void set_device_id(uint8_t device_id) noexcept;
    uint8_t get_device_id() const noexcept;
    
    void set_message_callback(MessageCallback callback);
    void set_message_received_callback(MessageReceivedCallback callback);
    void set_connections_changed_callback(ConnectionsChangedCallback callback);
    
    void store_connection(uint32_t destination_muid, std::shared_ptr<ClientConnection> connection);
    void remove_connection(uint32_t destination_muid);
    std::shared_ptr<ClientConnection> get_connection(uint32_t destination_muid) const;
    const std::unordered_map<uint32_t, std::shared_ptr<ClientConnection>>& get_connections() const;
    
    void processInput(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    uint32_t get_muid() const noexcept;
    DeviceInfo& get_device_info() const;
    MidiCIDeviceConfiguration& get_config() const;
    
    void set_sysex_sender(CIOutputSender sender);
    CIOutputSender get_ci_output_sender() const;

    void sendDiscovery();
    
    profiles::ProfileHostFacade& get_profile_host_facade();
    const profiles::ProfileHostFacade& get_profile_host_facade() const;
    
    properties::PropertyHostFacade& get_property_host_facade();
    const properties::PropertyHostFacade& get_property_host_facade() const;
    
    void set_logger(LoggerFunction logger);
    LoggerFunction get_logger() const;
    
    messages::Messenger& get_messenger();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
