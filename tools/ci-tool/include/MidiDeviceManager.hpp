#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <cstdint>

namespace ci_tool {

class MidiDeviceManager {
public:
    using SysExCallback = std::function<void(uint8_t group, const std::vector<uint8_t>& data)>;
    using SysExSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    MidiDeviceManager();
    ~MidiDeviceManager();
    
    MidiDeviceManager(const MidiDeviceManager&) = delete;
    MidiDeviceManager& operator=(const MidiDeviceManager&) = delete;
    
    void initialize();
    void shutdown();
    
    void set_sysex_callback(SysExCallback callback);
    void set_sysex_sender(SysExSender sender);
    
    bool send_sysex(uint8_t group, const std::vector<uint8_t>& data);
    
    void process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& data);
    
    std::vector<std::string> get_available_input_devices() const;
    std::vector<std::string> get_available_output_devices() const;
    
    bool set_input_device(const std::string& device_id);
    bool set_output_device(const std::string& device_id);
    
    std::string get_current_input_device() const;
    std::string get_current_output_device() const;
    
    bool is_initialized() const noexcept;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
