#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <variant>
#include "midicci/midicci.hpp"

namespace midicci {
class ProfileHostFacade;
class ProfileClientFacade;
class PropertyHostFacade;
class PropertyClientFacade;
class ClientConnection;

// Log data that can contain either a plain string or a structured MIDI-CI message
struct LogData {
    std::variant<std::string, std::reference_wrapper<const Message>> data;
    bool is_outgoing;
    
    LogData(const std::string& str, bool outgoing) : data(str), is_outgoing(outgoing) {}
    LogData(const Message& msg, bool outgoing) : data(std::cref(msg)), is_outgoing(outgoing) {}
    
    bool has_message() const { return std::holds_alternative<std::reference_wrapper<const Message>>(data); }
    const Message& get_message() const { return std::get<std::reference_wrapper<const Message>>(data); }
    const std::string& get_string() const { return std::get<std::string>(data); }
};

class MidiCIDevice {
public:
    typedef std::function<void(const Message&)> MessageCallback;
    typedef std::function<void(const Message&)> MessageReceivedCallback;
    typedef std::function<void()> ConnectionsChangedCallback;
    typedef std::function<bool(uint8_t group, const std::vector<uint8_t>& data)> CIOutputSender;
    typedef std::function<void(const LogData&)> LoggerFunction;
    typedef std::function<void(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& header)> PropertyChunkCallback;
    
    MidiCIDevice(uint32_t muid, MidiCIDeviceConfiguration& config, LoggerFunction logger = LoggerFunction{});
    ~MidiCIDevice();
    
    MidiCIDevice(const MidiCIDevice&) = delete;
    MidiCIDevice& operator=(const MidiCIDevice&) = delete;
    
    MidiCIDevice(MidiCIDevice&&) = default;
    MidiCIDevice& operator=(MidiCIDevice&&) = default;

    void set_message_callback(MessageCallback callback);
    void set_message_received_callback(MessageReceivedCallback callback);
    void set_connections_changed_callback(ConnectionsChangedCallback callback);
    void set_property_chunk_callback(PropertyChunkCallback callback);
    
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
    
    void notify_property_chunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& header);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
