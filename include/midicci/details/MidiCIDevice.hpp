#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include "midicci/midicci.hpp"

namespace midicci {
class ProfileHostFacade;
class ProfileClientFacade;
class PropertyHostFacade;
class PropertyClientFacade;
class ClientConnection;

class MidiCIDevice {
public:
    typedef std::function<void(const Message&)> MessageCallback;
    typedef std::function<void(const Message&)> MessageReceivedCallback;
    typedef std::function<void()> ConnectionsChangedCallback;
    typedef std::function<bool(uint8_t group, const std::vector<uint8_t>& data)> CIOutputSender;
    typedef std::function<void(const std::string&, bool)> LoggerFunction;
    
    MidiCIDevice(uint32_t muid, MidiCIDeviceConfiguration& config, LoggerFunction logger = LoggerFunction{});
    ~MidiCIDevice();
    
    MidiCIDevice(const MidiCIDevice&) = delete;
    MidiCIDevice& operator=(const MidiCIDevice&) = delete;
    
    MidiCIDevice(MidiCIDevice&&) = default;
    MidiCIDevice& operator=(MidiCIDevice&&) = default;

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
    
    ProfileHostFacade& get_profile_host_facade();
    const ProfileHostFacade& get_profile_host_facade() const;
    
    PropertyHostFacade& get_property_host_facade();
    const PropertyHostFacade& get_property_host_facade() const;
    
    void set_logger(LoggerFunction logger);
    LoggerFunction get_logger() const;
    
    Messenger& get_messenger();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
