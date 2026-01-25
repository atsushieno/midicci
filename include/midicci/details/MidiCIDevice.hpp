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

    bool hasMessage() const { return std::holds_alternative<std::reference_wrapper<const Message>>(data); }
    const Message& getMessage() const { return std::get<std::reference_wrapper<const Message>>(data); }
    const std::string& getString() const { return std::get<std::string>(data); }
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

    void setMessageCallback(MessageCallback callback);
    void setMessageReceivedCallback(MessageReceivedCallback callback);
    void setConnectionsChangedCallback(ConnectionsChangedCallback callback);
    void setPropertyChunkCallback(PropertyChunkCallback callback);
    
    void storeConnection(uint32_t destination_muid, std::shared_ptr<ClientConnection> connection);
    void removeConnection(uint32_t destination_muid);
    std::shared_ptr<ClientConnection> getConnection(uint32_t destination_muid) const;
    const std::unordered_map<uint32_t, std::shared_ptr<ClientConnection>>& getConnections() const;
    
    void processInput(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    uint32_t getMuid() const noexcept;
    DeviceInfo& getDeviceInfo() const;
    MidiCIDeviceConfiguration& getConfig() const;
    
    void setSysexSender(CIOutputSender sender);
    CIOutputSender getCiOutputSender() const;

    void sendDiscovery();
    
    ProfileHostFacade& getProfileHostFacade();
    const ProfileHostFacade& getProfileHostFacade() const;
    
    PropertyHostFacade& getPropertyHostFacade();
    const PropertyHostFacade& getPropertyHostFacade() const;
    
    void setLogger(LoggerFunction logger);
    LoggerFunction getLogger() const;
    
    Messenger& getMessenger();
    
    void notifyPropertyChunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& header);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
