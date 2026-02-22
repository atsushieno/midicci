#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <cstdint>
#include <mutex>
#include <umppi/details/Ump.hpp>

namespace libremidi {
class midi_in;
class midi_out;
struct ump;
}

namespace midicci::tooling {

enum class VirtualPortDirection {
    In,
    Out
};

class MidiDeviceManager {
public:
    using SysExCallback = std::function<void(uint8_t group, umppi::UmpWordSpan words)>;
    using UmpListener = std::function<void(umppi::UmpWordSpan, uint64_t)>;
    
    MidiDeviceManager();
    ~MidiDeviceManager();
    
    MidiDeviceManager(const MidiDeviceManager&) = delete;
    MidiDeviceManager& operator=(const MidiDeviceManager&) = delete;
    
    void initialize();
    void shutdown();
    
    void set_sysex_callback(SysExCallback callback);
    bool send_sysex(uint8_t group, umppi::UmpWordSpan words);
    void send_ump(umppi::UmpWordSpan words, uint64_t timestamp_ns);
    
    void process_incoming_sysex(uint8_t group, umppi::UmpWordSpan words);
    void add_ump_listener(UmpListener listener);
    void clear_ump_listeners();
    
    std::vector<std::string> get_available_input_devices() const;
    std::vector<std::string> get_available_output_devices() const;
    
    bool set_input_device(const std::string& device_id);
    bool set_output_device(const std::string& device_id);
    
    std::string get_current_input_device() const;
    std::string get_current_output_device() const;
    
    bool is_initialized() const noexcept;
    
    void add_input_opened_callback(std::function<void()> callback);
    void add_output_opened_callback(std::function<void()> callback);

    void set_log_callback(std::function<void(const std::string&, VirtualPortDirection)> callback);

    void set_virtual_input_name(const std::string& name);
    void set_virtual_output_name(const std::string& name);
    std::string get_virtual_input_name() const;
    std::string get_virtual_output_name() const;
    void enable_virtual_ports(bool enabled);
    bool virtual_ports_enabled() const noexcept;
    void send_to_virtual_output(const libremidi::ump& packet);
    void add_note_event_callback(std::function<void(int note, int velocity, bool is_pressed)> callback);
    
private:
    bool initialized_;
    SysExCallback sysex_callback_;
    std::vector<UmpListener> ump_listeners_;
    std::string current_input_device_;
    std::string current_output_device_;
    
    std::unique_ptr<libremidi::midi_in> midi_input_;
    std::unique_ptr<libremidi::midi_out> midi_output_;

    std::unique_ptr<libremidi::midi_in> virtual_midi_input_;
    std::unique_ptr<libremidi::midi_out> virtual_midi_output_;
    std::string virtual_input_name_;
    std::string virtual_output_name_;
    bool virtual_ports_enabled_;
    
    std::vector<std::function<void()>> midi_input_opened_;
    std::vector<std::function<void()>> midi_output_opened_;
    std::vector<std::function<void(int, int, bool)>> note_event_callbacks_;
    
    mutable std::recursive_mutex mutex_;
    std::function<void(const std::string&, VirtualPortDirection)> log_callback_;

    void handle_input_message(libremidi::ump&& packet, bool from_virtual);
    void forward_to_virtual_output(const libremidi::ump& packet);
    void forward_to_physical_output(const libremidi::ump& packet);
    void open_virtual_ports_locked();
    void close_virtual_ports_locked();
    void reopen_virtual_ports_locked();
    void notify_ump_listeners(umppi::UmpWordSpan words, uint64_t timestamp_ns);
    static std::string format_ump_packet(const libremidi::ump& packet);
    void log_virtual_event(const std::string& message, VirtualPortDirection direction);
    bool extract_note_event(const libremidi::ump& packet, int& note, int& velocity, bool& is_pressed) const;
};

} // namespace ci_tool
