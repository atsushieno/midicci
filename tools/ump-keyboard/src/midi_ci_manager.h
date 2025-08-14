#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <cstdint>
#include <map>
#include <chrono>
#include <mutex>
#include <midicci/midicci.hpp>
#include <midicci/details/commonproperties/StandardProperties.hpp>

struct MidiCIDeviceInfo {
    uint32_t muid;
    std::string device_name;
    std::string manufacturer;
    std::string model;
    std::string version;
    uint8_t supported_features;
    uint32_t max_sysex_size;
    bool endpoint_ready;  // True when EndpointReply has been received
    
    MidiCIDeviceInfo(uint32_t m, const std::string& name, const std::string& mfg, const std::string& mod, 
                     const std::string& ver, uint8_t features, uint32_t sysex_size)
        : muid(m), device_name(name), manufacturer(mfg), model(mod), version(ver), 
          supported_features(features), max_sysex_size(sysex_size), endpoint_ready(false) {}
    
    std::string getDisplayName() const {
        return model + " (" + manufacturer + ")";
    }
    
    std::string getFullInfo() const {
        return "MUID: 0x" + std::to_string(muid) + ", " + 
               manufacturer + " " + model + " v" + version;
    }
};

class MidiCIManager {
public:
    using LogCallback = std::function<void(const std::string&)>;
    using SysExSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    using DevicesChangedCallback = std::function<void()>;
    
    MidiCIManager();
    ~MidiCIManager();
    
    // Initialization and cleanup
    bool initialize(uint32_t muid = 0);
    void shutdown();
    
    // MIDI message processing
    void processMidi1SysEx(const std::vector<uint8_t>& sysex_data);
    void processUmpSysEx(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    // Device management
    void sendDiscovery();
    std::vector<std::string> getDiscoveredDevices() const;
    std::vector<MidiCIDeviceInfo> getDiscoveredDeviceDetails() const;
    MidiCIDeviceInfo* getDeviceByMuid(uint32_t muid);
    
    // Configuration
    void setSysExSender(SysExSender sender);
    void setLogCallback(LogCallback callback);
    void setDevicesChangedCallback(DevicesChangedCallback callback);
    
    // Reset and cleanup
    void clearDiscoveredDevices();
    
    // Device information
    uint32_t getMuid() const;
    std::string getDeviceName() const;
    bool isInitialized() const;
    
    // Property management - simplified API using StandardPropertiesExtensions
    std::optional<std::vector<midicci::commonproperties::MidiCIControl>> getAllCtrlList(uint32_t muid);
    std::optional<std::vector<midicci::commonproperties::MidiCIProgram>> getProgramList(uint32_t muid);
    void setPropertiesChangedCallback(std::function<void(uint32_t)> callback);

private:
    std::unique_ptr<midicci::MidiCIDevice> device_;
    std::unique_ptr<midicci::MidiCIDeviceConfiguration> config_;
    
    SysExSender sysex_sender_;
    LogCallback log_callback_;
    DevicesChangedCallback devices_changed_callback_;
    std::function<void(uint32_t)> properties_changed_callback_;
    uint32_t muid_;
    bool initialized_;
    
    std::vector<MidiCIDeviceInfo> discovered_devices_;
    
    // Property request tracking to prevent infinite loops
    struct PendingPropertyRequest {
        uint32_t muid;
        std::string property_name;
        std::chrono::steady_clock::time_point request_time;
        
        PendingPropertyRequest(uint32_t m, const std::string& prop) 
            : muid(m), property_name(prop), request_time(std::chrono::steady_clock::now()) {}
    };
    std::vector<PendingPropertyRequest> pending_property_requests_;
    
    // Note: Remote device access is now handled through ClientConnection objects
    // obtained via device_->get_connection(muid) - no need for separate storage
    
    // Device configuration helpers
    void setupDeviceConfiguration();
    void setupCallbacks();

    void setupPropertyCallbacks(uint32_t muid);
    bool isPropertyRequestPending(uint32_t muid, const std::string& property_name);
    void addPendingPropertyRequest(uint32_t muid, const std::string& property_name);
    void removePendingPropertyRequest(uint32_t muid, const std::string& property_name);
    void cleanupExpiredPropertyRequests();
    
    // Logging helper
    void log(const std::string& message, bool is_outgoing = false);
    
    // Thread synchronization for cross-thread access
    mutable std::recursive_mutex midi_ci_mutex_;
};