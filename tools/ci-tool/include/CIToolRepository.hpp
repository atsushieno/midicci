#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <mutex>

namespace midi_ci {
namespace messages {
class Message;
}
}

namespace ci_tool {

enum class MessageDirection {
    In,
    Out
};

struct LogEntry {
    std::chrono::system_clock::time_point timestamp;
    MessageDirection direction;
    std::string message;
    
    LogEntry(MessageDirection dir, const std::string& msg);
};

class MidiDeviceManager;
class CIDeviceManager;

class CIToolRepository {
public:
    using LogCallback = std::function<void(const LogEntry&)>;
    
    CIToolRepository();
    ~CIToolRepository();
    
    CIToolRepository(const CIToolRepository&) = delete;
    CIToolRepository& operator=(const CIToolRepository&) = delete;
    
    void log(const std::string& message, MessageDirection direction);
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
    
    static constexpr const char* DEFAULT_CONFIG_FILE = "midi-ci-tool.settings.json";
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
