#include "MidiDeviceManager.hpp"
#include <libremidi/libremidi.hpp>
#include <iostream>

namespace midicci::tooling {

MidiDeviceManager::MidiDeviceManager() : initialized_(false) {}

MidiDeviceManager::~MidiDeviceManager() {
    shutdown();
}

void MidiDeviceManager::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!initialized_) {
        initialized_ = true;
        std::cout << "MidiDeviceManager initialized (transport-agnostic)" << std::endl;
    }
}

void MidiDeviceManager::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (initialized_) {
        if (midi_input_) {
            midi_input_->close_port();
            midi_input_.reset();
        }
        if (midi_output_) {
            midi_output_->close_port();
            midi_output_.reset();
        }
        
        initialized_ = false;
        std::cout << "MidiDeviceManager shutdown" << std::endl;
    }
}

void MidiDeviceManager::set_sysex_callback(SysExCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sysex_callback_ = std::move(callback);
}

void MidiDeviceManager::set_ci_output_sender(CIOutputSender sender) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ci_output_sender_ = std::move(sender);
}

bool MidiDeviceManager::send_sysex(uint8_t group, const std::vector<uint32_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (ci_output_sender_) {
        return ci_output_sender_(group, data);
    }
    
    if (midi_output_) {
        try {
            // Send UMP data directly
            midi_output_->send_ump(data.data(), data.size());
            return true;
        } catch (const std::exception& e) {
            std::cerr << "Error sending UMP message: " << e.what() << std::endl;
        }
    }
    return false;
}

void MidiDeviceManager::process_incoming_sysex(uint8_t group, const std::vector<uint32_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (sysex_callback_) {
        sysex_callback_(group, data);
    }
}

std::vector<std::string> MidiDeviceManager::get_available_input_devices() const {
    std::vector<std::string> devices;
    try {
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
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
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
        for(const libremidi::output_port& port : obs.get_output_ports()) {
            devices.push_back(port.port_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error enumerating output devices: " << e.what() << std::endl;
    }
    return devices;
}

bool MidiDeviceManager::set_input_device(const std::string& device_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (midi_input_) {
        midi_input_->close_port();
        midi_input_.reset();
    }
    
    if (!device_id.empty()) {
        try {
            libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
            auto input_ports = obs.get_input_ports();
            
            for (const auto& port : input_ports) {
                if (port.port_name == device_id) {
                    libremidi::ump_input_configuration config{
                        .on_message = [this](libremidi::ump&& ump_packet) {
                            std::vector<uint32_t> data{ump_packet.data[0], ump_packet.data[1], ump_packet.data[2], ump_packet.data[3]};
                            process_incoming_sysex(0, data);
                        }
                    };
                    config.ignore_sysex = false;
                    
                    midi_input_ = std::make_unique<libremidi::midi_in>(config, libremidi::midi2::in_default_configuration());
                    
                    if (auto err = midi_input_->open_port(port); err != stdx::error{}) {
                        auto msg = err.message();
                        std::cerr << "Error opening input port: " << std::string(msg.data(), msg.size()) << std::endl;
                        midi_input_.reset();
                        return false;
                    }
                    
                    current_input_device_ = device_id;
                    
                    for (const auto& callback : midi_input_opened_) {
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
    
    current_input_device_ = device_id;
    return true;
}

bool MidiDeviceManager::set_output_device(const std::string& device_id) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    if (midi_output_) {
        midi_output_->close_port();
        midi_output_.reset();
    }
    
    if (!device_id.empty()) {
        try {
            libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
            auto output_ports = obs.get_output_ports();
            
            for (const auto& port : output_ports) {
                if (port.port_name == device_id) {
                    midi_output_ = std::make_unique<libremidi::midi_out>(libremidi::output_configuration{}, libremidi::midi2::out_default_configuration());
                    
                    if (auto err = midi_output_->open_port(port); err != stdx::error{}) {
                        auto msg = err.message();
                        std::cerr << "Error opening output port: " << std::string(msg.data(), msg.size()) << std::endl;
                        midi_output_.reset();
                        return false;
                    }
                    
                    current_output_device_ = device_id;
                    
                    for (const auto& callback : midi_output_opened_) {
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
    
    current_output_device_ = device_id;
    return true;
}

std::string MidiDeviceManager::get_current_input_device() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return current_input_device_;
}

std::string MidiDeviceManager::get_current_output_device() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return current_output_device_;
}

bool MidiDeviceManager::is_initialized() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return initialized_;
}

void MidiDeviceManager::add_input_opened_callback(std::function<void()> callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    midi_input_opened_.push_back(std::move(callback));
}

void MidiDeviceManager::add_output_opened_callback(std::function<void()> callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    midi_output_opened_.push_back(std::move(callback));
}

} // namespace ci_tool
