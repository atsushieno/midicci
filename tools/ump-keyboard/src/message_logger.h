#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <mutex>

namespace midicci::keyboard {

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

class MessageLogger {
public:
    using LogCallback = std::function<void(const LogEntry&)>;
    
    MessageLogger();
    ~MessageLogger();
    
    MessageLogger(const MessageLogger&) = delete;
    MessageLogger& operator=(const MessageLogger&) = delete;
    
    void log(const std::string& message, MessageDirection direction);
    void add_log_callback(LogCallback callback);
    void remove_log_callback(LogCallback callback);
    
    std::vector<LogEntry> get_logs() const;
    void clear_logs();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace