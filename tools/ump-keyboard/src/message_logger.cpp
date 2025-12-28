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

    // Recording state
    bool recording_enabled_ = false;
    std::vector<uint8_t> recorded_inputs_;
    std::vector<uint8_t> recorded_outputs_;
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
    std::string log_message = message.get_log_message();
    uint32_t source_muid = message.get_source_muid();
    uint32_t destination_muid = message.get_destination_muid();
    
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

void MessageLogger::set_recording_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->recording_enabled_ = enabled;
}

bool MessageLogger::is_recording_enabled() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->recording_enabled_;
}

void MessageLogger::record_input_sysex(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (!pimpl_->recording_enabled_) return;
    pimpl_->recorded_inputs_.insert(pimpl_->recorded_inputs_.end(), data.begin(), data.end());
}

void MessageLogger::record_output_sysex(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (!pimpl_->recording_enabled_) return;
    pimpl_->recorded_outputs_.insert(pimpl_->recorded_outputs_.end(), data.begin(), data.end());
}

std::vector<uint8_t> MessageLogger::get_recorded_inputs() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->recorded_inputs_;
}

std::vector<uint8_t> MessageLogger::get_recorded_outputs() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->recorded_outputs_;
}

void MessageLogger::clear_recorded() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->recorded_inputs_.clear();
    pimpl_->recorded_outputs_.clear();
}

} // namespace
