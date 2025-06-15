#include "MidiDeviceManager.hpp"
#include <libremidi/libremidi.hpp>
#include <mutex>
#include <iostream>

namespace ci_tool {

class MidiDeviceManager::Impl {
public:
    Impl() : initialized_(false) {}
    
    bool initialized_;
    SysExCallback sysex_callback_;
    SysExSender sysex_sender_;
    std::string current_input_device_;
    std::string current_output_device_;
    mutable std::mutex mutex_;
};

MidiDeviceManager::MidiDeviceManager() : pimpl_(std::make_unique<Impl>()) {}

MidiDeviceManager::~MidiDeviceManager() {
    shutdown();
}

void MidiDeviceManager::initialize() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (!pimpl_->initialized_) {
        pimpl_->initialized_ = true;
        std::cout << "MidiDeviceManager initialized (transport-agnostic)" << std::endl;
    }
}

void MidiDeviceManager::shutdown() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_) {
        pimpl_->initialized_ = false;
        std::cout << "MidiDeviceManager shutdown" << std::endl;
    }
}

void MidiDeviceManager::set_sysex_callback(SysExCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->sysex_callback_ = std::move(callback);
}

void MidiDeviceManager::set_sysex_sender(SysExSender sender) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->sysex_sender_ = std::move(sender);
}

bool MidiDeviceManager::send_sysex(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->sysex_sender_) {
        return pimpl_->sysex_sender_(group, data);
    }
    return false;
}

void MidiDeviceManager::process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (pimpl_->sysex_callback_) {
        pimpl_->sysex_callback_(group, data);
    }
}

std::vector<std::string> MidiDeviceManager::get_available_input_devices() const {
    std::vector<std::string> devices;
    try {
        libremidi::observer obs;
        for(const libremidi::input_port& port : obs.get_input_ports()) {
            devices.push_back(port.port_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error enumerating input devices: " << e.what() << std::endl;
    }
    return devices;
}

std::vector<std::string> MidiDeviceManager::get_available_output_devices() const {
    std::vector<std::string> devices;
    try {
        libremidi::observer obs;
        for(const libremidi::output_port& port : obs.get_output_ports()) {
            devices.push_back(port.port_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error enumerating output devices: " << e.what() << std::endl;
    }
    return devices;
}

bool MidiDeviceManager::set_input_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->current_input_device_ = device_id;
    std::cout << "Set input device: " << device_id << std::endl;
    return true;
}

bool MidiDeviceManager::set_output_device(const std::string& device_id) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->current_output_device_ = device_id;
    std::cout << "Set output device: " << device_id << std::endl;
    return true;
}

std::string MidiDeviceManager::get_current_input_device() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->current_input_device_;
}

std::string MidiDeviceManager::get_current_output_device() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->current_output_device_;
}

bool MidiDeviceManager::is_initialized() const noexcept {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->initialized_;
}

} // namespace ci_tool
