#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <cstdint>
#include <mutex>

namespace libremidi {
class midi_in;
class midi_out;
}

namespace midicci::tooling {

class MidiDeviceManager {
public:
    using SysExCallback = std::function<void(uint8_t group, const std::vector<uint32_t>& data)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint32_t>& data)>;
    
    MidiDeviceManager();
    ~MidiDeviceManager();
    
    MidiDeviceManager(const MidiDeviceManager&) = delete;
    MidiDeviceManager& operator=(const MidiDeviceManager&) = delete;
    
    void initialize();
    void shutdown();
    
    void set_sysex_callback(SysExCallback callback);
    void set_ci_output_sender(CIOutputSender sender);
    
    bool send_sysex(uint8_t group, const std::vector<uint32_t>& data);
    
    void process_incoming_sysex(uint8_t group, const std::vector<uint32_t>& data);
    
    std::vector<std::string> get_available_input_devices() const;
    std::vector<std::string> get_available_output_devices() const;
    
    bool set_input_device(const std::string& device_id);
    bool set_output_device(const std::string& device_id);
    
    std::string get_current_input_device() const;
    std::string get_current_output_device() const;
    
    bool is_initialized() const noexcept;
    
    void add_input_opened_callback(std::function<void()> callback);
    void add_output_opened_callback(std::function<void()> callback);
    
private:
    bool initialized_;
    SysExCallback sysex_callback_;
    CIOutputSender ci_output_sender_;
    std::string current_input_device_;
    std::string current_output_device_;
    
    std::unique_ptr<libremidi::midi_in> midi_input_;
    std::unique_ptr<libremidi::midi_out> midi_output_;
    
    std::vector<std::function<void()>> midi_input_opened_;
    std::vector<std::function<void()>> midi_output_opened_;
    
    mutable std::recursive_mutex mutex_;
};

} // namespace ci_tool
