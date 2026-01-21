#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>
#include "midicci/midicci.hpp"

namespace midicci::musicdevice {

// Abstract interface for receiving MIDI input
class MusicDeviceInputReceiver {
public:
    using InputCallback = std::function<void(const uint8_t*, size_t, size_t, uint64_t)>;
    
    virtual ~MusicDeviceInputReceiver() = default;
    virtual void addInputReceiver(InputCallback callback) = 0;
    virtual void removeInputReceiver(InputCallback callback) = 0;
};

// Abstract interface for sending MIDI output  
class MusicDeviceOutputSender {
public:
    virtual ~MusicDeviceOutputSender() = default;
    virtual void send(const uint8_t* bytes, size_t offset, size_t length, uint64_t timestamp_ns) = 0;
};

// Helper implementation that wraps callback-based MIDI I/O
class CallbackMusicDeviceInputReceiver : public MusicDeviceInputReceiver {
public:
    explicit CallbackMusicDeviceInputReceiver(MidiInputListenerAdder listener_adder);
    
    void addInputReceiver(InputCallback callback) override;
    void removeInputReceiver(InputCallback callback) override;
    
private:
    std::vector<InputCallback> input_receivers_;
    MidiInputListenerAdder listener_adder_;
};

class CallbackMusicDeviceOutputSender : public MusicDeviceOutputSender {
public:
    explicit CallbackMusicDeviceOutputSender(std::function<void(const uint8_t*, size_t, size_t, uint64_t)> output_sender);
    
    void send(const uint8_t* bytes, size_t offset, size_t length, uint64_t timestamp_ns) override;
    
private:
    std::function<void(const uint8_t*, size_t, size_t, uint64_t)> output_sender_;
};

// Helps determine which MIDI-CI to connect among discovered endpoints
class MusicDeviceConnector {
public:
    using EndpointSelector = std::function<uint32_t(const MidiCIDevice&)>;
    
    MusicDeviceConnector(
        std::shared_ptr<MusicDeviceInputReceiver> receiver,
        std::shared_ptr<MusicDeviceOutputSender> sender,
        std::shared_ptr<MidiCISession> ci_session
    );
    
    // Asynchronous connection with timeout (returns connection result via callback)
    using ConnectionCallback = std::function<void(std::unique_ptr<class MusicDevice>, const std::string& error)>;
    void connectAsync(ConnectionCallback callback);
    
    // Synchronous connection with timeout
    std::unique_ptr<class MusicDevice> connect(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(10000)
    );
    
    void send(const uint8_t* data, size_t offset, size_t length, uint64_t timestamp_ns);
    
    // Configure discovery parameters
    void setEndpointSelector(EndpointSelector selector);
    void setDiscoveryWait(std::chrono::milliseconds wait);
    void setDiscoveryTimeout(std::chrono::milliseconds timeout);
    
private:
    std::shared_ptr<MusicDeviceInputReceiver> receiver_;
    std::shared_ptr<MusicDeviceOutputSender> sender_;
    std::shared_ptr<MidiCISession> ci_session_;
    
    EndpointSelector select_target_endpoint_;
    std::chrono::milliseconds discovery_wait_;
    std::chrono::milliseconds discovery_timeout_;
};

// High-level MIDI device with MIDI-CI property access
class MusicDevice {
public:
    MusicDevice(
        std::shared_ptr<MusicDeviceOutputSender> sender,
        uint32_t target_muid,
        std::shared_ptr<MidiCISession> ci_session
    );
    
    ~MusicDevice() = default;
    MusicDevice(const MusicDevice&) = delete;
    MusicDevice& operator=(const MusicDevice&) = delete;
    MusicDevice(MusicDevice&&) = default;
    MusicDevice& operator=(MusicDevice&&) = default;
    
    // MIDI-CI connection info
    uint32_t getTargetMuid() const noexcept { return target_muid_; }
    
    // Access to MIDI-CI facilities
    std::shared_ptr<ClientConnection> getConnection() const;
    
    // Convenience accessors for MIDI-CI information
    // Note: These return nullopt if connection is not established
    std::optional<DeviceInfo> getDeviceInfo() const;
    // TODO: Add channel list, JSON schema, and property accessors when available
    
    // Send MIDI data
    void send(const uint8_t* data, size_t offset, size_t length, uint64_t timestamp_ns);

    // Property binary getter accessor (following Kotlin propertyBinaryGetter)
    using PropertyBinaryGetter = std::function<std::vector<uint8_t>(const std::string& property_id, const std::string& res_id)>;
    void setPropertyBinaryGetter(PropertyBinaryGetter getter);
    PropertyBinaryGetter getPropertyBinaryGetter() const;

    // Property binary setter accessor (following Kotlin propertyBinarySetter)
    using PropertyBinarySetter = std::function<bool(const std::string& property_id, const std::string& res_id, const std::string& media_type, const std::vector<uint8_t>& body)>;
    void setPropertyBinarySetter(PropertyBinarySetter setter);
    PropertyBinarySetter getPropertyBinarySetter() const;

private:
    std::shared_ptr<MusicDeviceOutputSender> sender_;
    uint32_t target_muid_;
    std::shared_ptr<MidiCISession> ci_session_;
};

} // namespace midicci::musicdevice