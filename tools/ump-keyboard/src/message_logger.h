#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <mutex>

// Forward declaration for MIDI-CI message
namespace midicci {
    class Message;
}

namespace midicci::keyboard {

enum class MessageDirection {
    In,
    Out
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    MessageDirection direction;
    std::string message;
    uint32_t source_muid;
    uint32_t destination_muid;
    
    LogEntry(MessageDirection dir, const std::string& msg, uint32_t src_muid = 0, uint32_t dest_muid = 0);
};

class MessageLogger {
public:
    using LogCallback = std::function<void(const LogEntry&)>;
    
    MessageLogger();
    ~MessageLogger();
    
    MessageLogger(const MessageLogger&) = delete;
    MessageLogger& operator=(const MessageLogger&) = delete;
    
    void log(const std::string& message, MessageDirection direction, uint32_t source_muid = 0, uint32_t destination_muid = 0);
    void log_midi_ci_message(const midicci::Message& message, MessageDirection direction);
    void add_log_callback(LogCallback callback);
    void remove_log_callback(LogCallback callback);
    
    std::vector<LogEntry> get_logs() const;
    void clear_logs();

    // Recording of raw SysEx bytes
    void set_recording_enabled(bool enabled);
    bool is_recording_enabled() const;
    void record_input_sysex(const std::vector<uint8_t>& data);
    void record_output_sysex(const std::vector<uint8_t>& data);
    std::vector<uint8_t> get_recorded_inputs() const;
    std::vector<uint8_t> get_recorded_outputs() const;
    void clear_recorded();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
