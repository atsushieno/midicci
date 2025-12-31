#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <mutex>
#include "MidiDeviceManager.hpp"
#include "CIDeviceManager.hpp"

namespace midicci::tooling {

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

class CIToolRepository {
public:
    using LogCallback = std::function<void(const LogEntry&)>;
    
    CIToolRepository();
    ~CIToolRepository();
    
    CIToolRepository(const CIToolRepository&) = delete;
    CIToolRepository& operator=(const CIToolRepository&) = delete;
    
    void log(const std::string& message, MessageDirection direction, uint32_t source_muid = 0, uint32_t destination_muid = 0);
    void add_log_callback(LogCallback callback);
    void remove_log_callback(LogCallback callback);
    
    std::vector<LogEntry> get_logs() const;
    void clear_logs();
    
    uint32_t get_muid() const noexcept;
    
    std::shared_ptr<MidiDeviceManager> get_midi_device_manager() const;
    std::shared_ptr<CIDeviceManager> get_ci_device_manager() const;
    
    void load_config(const std::string& filename);
    void save_config(const std::string& filename);
    void load_default_config();
    void save_default_config();
    
    static constexpr const char* DEFAULT_CONFIG_FILE = "midi-ci-tool.settings.json_ish";
    
    // Recording of raw SysEx bytes
    void set_recording_enabled(bool enabled);
    bool is_recording_enabled() const;
    void record_input_sysex(const std::vector<uint8_t>& data);
    void record_output_sysex(const std::vector<uint8_t>& data);
    std::vector<uint8_t> get_recorded_inputs() const;
    std::vector<uint8_t> get_recorded_outputs() const;
    void clear_recorded();

    // Recording of raw UMP words
    void record_input_ump_words(const std::vector<uint32_t>& words);
    void record_output_ump_words(const std::vector<uint32_t>& words);
    std::vector<uint32_t> get_recorded_input_ump_words() const;
    std::vector<uint32_t> get_recorded_output_ump_words() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
