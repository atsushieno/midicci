#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>

namespace midi_ci {

namespace transport {
class SysExTransport;
}

namespace messages {
struct DeviceInfo;
}

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
struct DeviceConfig;

class MidiCIDevice {
public:
    using MessageCallback = std::function<void(const Message&)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    MidiCIDevice(uint32_t muid = 0x12345678);
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
    
    std::shared_ptr<ClientConnection> create_connection(uint8_t destination_id);
    void remove_connection(uint8_t destination_id);
    std::shared_ptr<ClientConnection> get_connection(uint8_t destination_id) const;
    const std::unordered_map<uint8_t, std::shared_ptr<ClientConnection>>& get_connections() const;
    
    void processInput(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    uint32_t get_muid() const noexcept;
    messages::DeviceInfo get_device_info() const;
    DeviceConfig get_config() const;
    
    void set_sysex_sender(CIOutputSender sender);
    CIOutputSender get_ci_output_sender() const;
    void set_sysex_transport(std::unique_ptr<transport::SysExTransport> transport);
    
    void sendDiscovery();
    
    profiles::ProfileHostFacade& get_profile_host_facade();
    const profiles::ProfileHostFacade& get_profile_host_facade() const;
    
    properties::PropertyHostFacade& get_property_host_facade();
    const properties::PropertyHostFacade& get_property_host_facade() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
