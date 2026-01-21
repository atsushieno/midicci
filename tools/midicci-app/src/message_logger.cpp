#include "message_logger.h"
#include "midicci/midicci.hpp"
#include <algorithm>

namespace midicci::keyboard {

LogEntry::LogEntry(MessageDirection dir, const std::string& msg, uint32_t src_muid, uint32_t dest_muid)
    : timestamp(std::chrono::system_clock::now()), direction(dir), message(msg), source_muid(src_muid), destination_muid(dest_muid) {}

class MessageLogger::Impl {
public:
    std::vector<LogEntry> logs_;
    std::vector<LogCallback> log_callbacks_;
    mutable std::mutex mutex_;
};

MessageLogger::MessageLogger() : pimpl_(std::make_unique<Impl>()) {
}

MessageLogger::~MessageLogger() = default;

void MessageLogger::log(const std::string& message, MessageDirection direction, uint32_t source_muid, uint32_t destination_muid) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    LogEntry entry(direction, message, source_muid, destination_muid);
    pimpl_->logs_.push_back(entry);
    
    for (const auto& callback : pimpl_->log_callbacks_) {
        callback(entry);
    }
}

void MessageLogger::log_midi_ci_message(const midicci::Message& message, MessageDirection direction) {
    std::string log_message = message.getLogMessage();
    uint32_t source_muid = message.getSourceMuid();
    uint32_t destination_muid = message.getDestinationMuid();
    
    log(log_message, direction, source_muid, destination_muid);
}

void MessageLogger::add_log_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->log_callbacks_.push_back(std::move(callback));
}

void MessageLogger::remove_log_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->log_callbacks_.begin(), pimpl_->log_callbacks_.end(),
        [&callback](const LogCallback& cb) {
            return cb.target<void(const LogEntry&)>() == callback.target<void(const LogEntry&)>();
        });
    if (it != pimpl_->log_callbacks_.end()) {
        pimpl_->log_callbacks_.erase(it);
    }
}

std::vector<LogEntry> MessageLogger::get_logs() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->logs_;
}

void MessageLogger::clear_logs() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->logs_.clear();
}

} // namespace