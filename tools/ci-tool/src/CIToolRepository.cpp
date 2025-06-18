#include "CIToolRepository.hpp"
#include "MidiDeviceManager.hpp"
#include "CIDeviceManager.hpp"
#include "midi-ci/json/Json.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include <random>
#include <fstream>
#include <iostream>

namespace ci_tool {

LogEntry::LogEntry(MessageDirection dir, const std::string& msg)
    : timestamp(std::chrono::system_clock::now()), direction(dir), message(msg) {}

class CIToolRepository::Impl {
public:
    Impl() : muid_(generate_muid()) {
        midi_device_manager_ = std::make_shared<MidiDeviceManager>();
    }
    
    uint32_t generate_muid() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
        return dis(gen) & 0x7F7F7F7F;
    }
    
    CIToolRepository* parent_;
    midi_ci::core::DeviceConfig config_{};
    std::vector<LogEntry> logs_;
    std::vector<LogCallback> log_callbacks_;
    mutable std::mutex mutex_;
    
    uint32_t muid_;
    std::shared_ptr<MidiDeviceManager> midi_device_manager_;
    std::shared_ptr<CIDeviceManager> ci_device_manager_;
};

CIToolRepository::CIToolRepository() : pimpl_(std::make_unique<Impl>()) {
    pimpl_->parent_ = this;
    pimpl_->ci_device_manager_ = std::make_shared<CIDeviceManager>(*this, pimpl_->config_, pimpl_->midi_device_manager_);
}

CIToolRepository::~CIToolRepository() = default;

void CIToolRepository::log(const std::string& message, MessageDirection direction) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    LogEntry entry(direction, message);
    pimpl_->logs_.push_back(entry);
    
    for (const auto& callback : pimpl_->log_callbacks_) {
        callback(entry);
    }
}

void CIToolRepository::add_log_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->log_callbacks_.push_back(std::move(callback));
}

void CIToolRepository::remove_log_callback(LogCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->log_callbacks_.begin(), pimpl_->log_callbacks_.end(),
        [&callback](const LogCallback& cb) {
            return cb.target<void(const LogEntry&)>() == callback.target<void(const LogEntry&)>();
        });
    if (it != pimpl_->log_callbacks_.end()) {
        pimpl_->log_callbacks_.erase(it);
    }
}

std::vector<LogEntry> CIToolRepository::get_logs() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->logs_;
}

void CIToolRepository::clear_logs() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->logs_.clear();
}

uint32_t CIToolRepository::get_muid() const noexcept {
    return pimpl_->muid_;
}

std::shared_ptr<MidiDeviceManager> CIToolRepository::get_midi_device_manager() const {
    return pimpl_->midi_device_manager_;
}

std::shared_ptr<CIDeviceManager> CIToolRepository::get_ci_device_manager() const {
    return pimpl_->ci_device_manager_;
}

void CIToolRepository::load_config(const std::string& filename) {
    try {
        std::ifstream file(filename);
        if (!file.is_open()) {
            log("Failed to open config file: " + filename, MessageDirection::In);
            return;
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        file.close();
        
        auto json_val = midi_ci::json::JsonValue::parse_or_null(content);
        if (json_val.is_null()) {
            log("Failed to parse config file: " + filename, MessageDirection::In);
            return;
        }
        
        log("Loaded config from: " + filename, MessageDirection::In);
    } catch (const std::exception& ex) {
        log("Exception loading config: " + std::string(ex.what()), MessageDirection::In);
    }
}

void CIToolRepository::save_config(const std::string& filename) {
    try {
        midi_ci::json::JsonValue config = midi_ci::json::JsonValue::empty_object();
        config["muid"] = midi_ci::json::JsonValue(static_cast<int>(pimpl_->muid_));
        
        std::ofstream file(filename);
        if (!file.is_open()) {
            log("Failed to create config file: " + filename, MessageDirection::Out);
            return;
        }
        
        file << config.serialize();
        file.close();
        
        log("Saved config to: " + filename, MessageDirection::Out);
    } catch (const std::exception& ex) {
        log("Exception saving config: " + std::string(ex.what()), MessageDirection::Out);
    }
}

void CIToolRepository::load_default_config() {
    load_config(DEFAULT_CONFIG_FILE);
}

void CIToolRepository::save_default_config() {
    save_config(DEFAULT_CONFIG_FILE);
}

} // namespace ci_tool
