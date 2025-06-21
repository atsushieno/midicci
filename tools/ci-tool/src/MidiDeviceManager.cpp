#include "MidiDeviceManager.hpp"
#include <libremidi/libremidi.hpp>
#include <mutex>
#include <iostream>

namespace tooling {

class MidiDeviceManager::Impl {
public:
    Impl() : initialized_(false) {}
    
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

MidiDeviceManager::MidiDeviceManager() : pimpl_(std::make_unique<Impl>()) {}

MidiDeviceManager::~MidiDeviceManager() {
    shutdown();
}

void MidiDeviceManager::initialize() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->initialized_) {
        pimpl_->initialized_ = true;
        std::cout << "MidiDeviceManager initialized (transport-agnostic)" << std::endl;
    }
}

void MidiDeviceManager::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_) {
        if (pimpl_->midi_input_) {
            pimpl_->midi_input_->close_port();
            pimpl_->midi_input_.reset();
        }
        if (pimpl_->midi_output_) {
            pimpl_->midi_output_->close_port();
            pimpl_->midi_output_.reset();
        }
        
        pimpl_->initialized_ = false;
        std::cout << "MidiDeviceManager shutdown" << std::endl;
    }
}

void MidiDeviceManager::set_sysex_callback(SysExCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->sysex_callback_ = std::move(callback);
}

void MidiDeviceManager::set_ci_output_sender(CIOutputSender sender) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->ci_output_sender_ = std::move(sender);
}

bool MidiDeviceManager::send_sysex(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->ci_output_sender_) {
        return pimpl_->ci_output_sender_(group, data);
    }
    
    if (pimpl_->midi_output_) {
        try {
            std::vector<uint8_t> midi1_data;
            midi1_data.reserve(data.size() + 2);
            midi1_data.push_back(0xF0);
            midi1_data.insert(midi1_data.end(), data.begin(), data.end());
            midi1_data.push_back(0xF7);
            pimpl_->midi_output_->send_message(midi1_data);
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error sending MIDI message: " << e.what() << std::endl;
        }
    }
    return false;
}

void MidiDeviceManager::process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->sysex_callback_) {
        pimpl_->sysex_callback_(group, data);
    }
}

std::vector<std::string> MidiDeviceManager::get_available_input_devices() const {
    std::vector<std::string> devices;
    try {
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true});
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
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true});
        for(const libremidi::output_port& port : obs.get_output_ports()) {
            devices.push_back(port.port_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error enumerating output devices: " << e.what() << std::endl;
    }
    return devices;
}

bool MidiDeviceManager::set_input_device(const std::string& device_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->midi_input_) {
        pimpl_->midi_input_->close_port();
        pimpl_->midi_input_.reset();
    }
    
    if (!device_id.empty()) {
        try {
            libremidi::observer obs({ .track_hardware = true, .track_virtual = true});
            auto input_ports = obs.get_input_ports();
            
            for (const auto& port : input_ports) {
                if (port.port_name == device_id) {
                    libremidi::input_configuration config{
                        .on_message = [this](const libremidi::message& message) {
                            std::vector<uint8_t> data(message.bytes.begin(), message.bytes.end());
                            process_incoming_sysex(0, data);
                        },
                        .ignore_sysex = false
                    };
                    
                    pimpl_->midi_input_ = std::make_unique<libremidi::midi_in>(config);
                    
                    if (auto err = pimpl_->midi_input_->open_port(port); err != stdx::error{}) {
                        auto msg = err.message();
                        std::cerr << "Error opening input port: " << std::string(msg.data(), msg.size()) << std::endl;
                        pimpl_->midi_input_.reset();
                        return false;
                    }
                    
                    pimpl_->current_input_device_ = device_id;
                    
                    for (const auto& callback : pimpl_->midi_input_opened_) {
                        callback();
                    }
                    
                    std::cout << "Opened input device: " << device_id << std::endl;
                    return true;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error opening input device: " << e.what() << std::endl;
            return false;
        }
    }
    
    pimpl_->current_input_device_ = device_id;
    return true;
}

bool MidiDeviceManager::set_output_device(const std::string& device_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->midi_output_) {
        pimpl_->midi_output_->close_port();
        pimpl_->midi_output_.reset();
    }
    
    if (!device_id.empty()) {
        try {
            libremidi::observer obs({ .track_hardware = true, .track_virtual = true});
            auto output_ports = obs.get_output_ports();
            
            for (const auto& port : output_ports) {
                if (port.port_name == device_id) {
                    pimpl_->midi_output_ = std::make_unique<libremidi::midi_out>();
                    
                    if (auto err = pimpl_->midi_output_->open_port(port); err != stdx::error{}) {
                        auto msg = err.message();
                        std::cerr << "Error opening output port: " << std::string(msg.data(), msg.size()) << std::endl;
                        pimpl_->midi_output_.reset();
                        return false;
                    }
                    
                    pimpl_->current_output_device_ = device_id;
                    
                    for (const auto& callback : pimpl_->midi_output_opened_) {
                        callback();
                    }
                    
                    std::cout << "Opened output device: " << device_id << std::endl;
                    return true;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error opening output device: " << e.what() << std::endl;
            return false;
        }
    }
    
    pimpl_->current_output_device_ = device_id;
    return true;
}

std::string MidiDeviceManager::get_current_input_device() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->current_input_device_;
}

std::string MidiDeviceManager::get_current_output_device() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->current_output_device_;
}

bool MidiDeviceManager::is_initialized() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->initialized_;
}

void MidiDeviceManager::add_input_opened_callback(std::function<void()> callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->midi_input_opened_.push_back(std::move(callback));
}

void MidiDeviceManager::add_output_opened_callback(std::function<void()> callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->midi_output_opened_.push_back(std::move(callback));
}

} // namespace ci_tool
