#include "MidiDeviceManager.hpp"
#include <libremidi/libremidi.hpp>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
constexpr const char* kDefaultVirtualInputName = "MIDICCI App (In)";
constexpr const char* kDefaultVirtualOutputName = "MIDICCI App (Out)";
}

namespace midicci::tooling {

MidiDeviceManager::MidiDeviceManager()
    : initialized_(false),
      virtual_input_name_(kDefaultVirtualInputName),
      virtual_output_name_(kDefaultVirtualOutputName),
      virtual_ports_enabled_(true) {}

MidiDeviceManager::~MidiDeviceManager() {
    shutdown();
}

void MidiDeviceManager::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (!initialized_) {
        initialized_ = true;
        open_virtual_ports_locked();
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

        close_virtual_ports_locked();
        
        initialized_ = false;
        std::cout << "MidiDeviceManager shutdown" << std::endl;
    }
}

void MidiDeviceManager::set_sysex_callback(SysExCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sysex_callback_ = std::move(callback);
}

bool MidiDeviceManager::send_sysex(uint8_t group, umppi::UmpWordSpan words) {
    (void)group;
    if (words.empty()) {
        return false;
    }
    send_ump(words, 0);
    return true;
}

void MidiDeviceManager::process_incoming_sysex(uint8_t group, umppi::UmpWordSpan words) {
    if (sysex_callback_) {
        sysex_callback_(group, words);
    }
}

void MidiDeviceManager::add_ump_listener(UmpListener listener) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ump_listeners_.push_back(std::move(listener));
}

void MidiDeviceManager::clear_ump_listeners() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    ump_listeners_.clear();
}

void MidiDeviceManager::send_ump(umppi::UmpWordSpan words, uint64_t timestamp_ns) {
    if (words.empty()) {
        return;
    }

    bool sent = false;

    if (midi_output_) {
        try {
            midi_output_->send_ump(words.data(), words.size());
            sent = true;
        } catch (const std::exception& e) {
            std::cerr << "Error sending UMP message: " << e.what() << std::endl;
        }
    }

    if (virtual_ports_enabled_ && virtual_midi_output_) {
        try {
            virtual_midi_output_->send_ump(words.data(), words.size());
            std::ostringstream oss;
            oss << std::uppercase << std::hex << std::setfill('0');
            for (size_t i = 0; i < words.size(); ++i) {
                oss << "0x" << std::setw(8) << words[i];
                if (i + 1 < words.size()) {
                    oss << ' ';
                }
            }
            log_virtual_event("[virtual out] " + oss.str(), VirtualPortDirection::Out);
            sent = true;
        } catch (const std::exception& e) {
            std::cerr << "Error sending virtual UMP message: " << e.what() << std::endl;
        }
    }

    if (!sent) {
        std::cerr << "Unable to send UMP message (timestamp=" << timestamp_ns << ")" << std::endl;
    }
}

std::vector<std::string> MidiDeviceManager::get_available_input_devices() const {
    std::string ignored_virtual_name;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        ignored_virtual_name = virtual_input_name_;
    }

    std::vector<std::string> devices;
    try {
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
        for(const libremidi::input_port& port : obs.get_input_ports()) {
            if (!ignored_virtual_name.empty() && port.port_name == ignored_virtual_name) {
                continue;
            }
            devices.push_back(port.port_name);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error enumerating input devices: " << e.what() << std::endl;
    }
    return devices;
}

std::vector<std::string> MidiDeviceManager::get_available_output_devices() const {
    std::string ignored_virtual_name;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        ignored_virtual_name = virtual_output_name_;
    }

    std::vector<std::string> devices;
    try {
        libremidi::observer obs({ .track_hardware = true, .track_virtual = true}, libremidi::midi2::observer_default_configuration());
        for(const libremidi::output_port& port : obs.get_output_ports()) {
            if (!ignored_virtual_name.empty() && port.port_name == ignored_virtual_name) {
                continue;
            }
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
                            handle_input_message(std::move(ump_packet), false);
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

void MidiDeviceManager::set_log_callback(std::function<void(const std::string&, VirtualPortDirection)> callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    log_callback_ = std::move(callback);
}

void MidiDeviceManager::add_note_event_callback(std::function<void(int, int, bool)> callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    note_event_callbacks_.push_back(std::move(callback));
}

void MidiDeviceManager::set_virtual_input_name(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (virtual_input_name_ == name) {
        return;
    }
    virtual_input_name_ = name.empty() ? kDefaultVirtualInputName : name;
    reopen_virtual_ports_locked();
}

void MidiDeviceManager::set_virtual_output_name(const std::string& name) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (virtual_output_name_ == name) {
        return;
    }
    virtual_output_name_ = name.empty() ? kDefaultVirtualOutputName : name;
    reopen_virtual_ports_locked();
}

std::string MidiDeviceManager::get_virtual_input_name() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return virtual_input_name_;
}

std::string MidiDeviceManager::get_virtual_output_name() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return virtual_output_name_;
}

void MidiDeviceManager::enable_virtual_ports(bool enabled) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (virtual_ports_enabled_ == enabled) {
        return;
    }
    virtual_ports_enabled_ = enabled;
    if (virtual_ports_enabled_) {
        open_virtual_ports_locked();
    } else {
        close_virtual_ports_locked();
    }
}

bool MidiDeviceManager::virtual_ports_enabled() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return virtual_ports_enabled_;
}

void MidiDeviceManager::send_to_virtual_output(const libremidi::ump& packet) {
    forward_to_virtual_output(packet);
}

void MidiDeviceManager::handle_input_message(libremidi::ump&& packet, bool from_virtual) {
    std::vector<std::function<void(int, int, bool)>> callbacks;
    int note = 0;
    int velocity = 0;
    bool is_pressed = false;
    bool has_note_event = false;

    {
        size_t word_count = packet.size();
        if (word_count == 0) {
            word_count = 1;
        }
        std::vector<uint32_t> data(word_count);
        for (size_t i = 0; i < word_count; ++i) {
            data[i] = packet.data[i];
        }

        umppi::UmpWordSpan span{data.data(), data.size()};
        uint8_t group = static_cast<uint8_t>((data[0] >> 24) & 0xF);
        if (from_virtual) {
            log_virtual_event("[virtual in] " + format_ump_packet(packet), VirtualPortDirection::In);
        }
        notify_ump_listeners(span, 0);
        process_incoming_sysex(group, span);

        if (extract_note_event(packet, note, velocity, is_pressed)) {
            callbacks = note_event_callbacks_;
            has_note_event = true;
        }

        if (from_virtual) {
            forward_to_physical_output(packet);
        } else {
            forward_to_virtual_output(packet);
        }
    }

    if (has_note_event) {
        for (const auto& cb : callbacks) {
            cb(note, velocity, is_pressed);
        }
    }
}

void MidiDeviceManager::forward_to_virtual_output(const libremidi::ump& packet) {
    if (!virtual_ports_enabled_ || !virtual_midi_output_) {
        return;
    }
    try {
        virtual_midi_output_->send_ump(packet);
        log_virtual_event("[virtual out] " + format_ump_packet(packet), VirtualPortDirection::Out);
    } catch (const std::exception& e) {
        std::cerr << "Error forwarding to virtual output: " << e.what() << std::endl;
    }
}

void MidiDeviceManager::forward_to_physical_output(const libremidi::ump& packet) {
    if (!midi_output_) {
        return;
    }
    try {
        midi_output_->send_ump(packet);
    } catch (const std::exception& e) {
        std::cerr << "Error forwarding to output: " << e.what() << std::endl;
    }
}

void MidiDeviceManager::open_virtual_ports_locked() {
    if (!initialized_ || !virtual_ports_enabled_) {
        return;
    }

    if (!virtual_midi_input_) {
        libremidi::ump_input_configuration config{
            .on_message = [this](libremidi::ump&& packet) {
                handle_input_message(std::move(packet), true);
            }
        };
        config.ignore_sysex = false;
        virtual_midi_input_ = std::make_unique<libremidi::midi_in>(config, libremidi::midi2::in_default_configuration());
        if (auto err = virtual_midi_input_->open_virtual_port(virtual_input_name_); err != stdx::error{}) {
            auto msg = err.message();
            std::cerr << "Error opening virtual input port: " << std::string(msg.data(), msg.size()) << std::endl;
            virtual_midi_input_.reset();
        }
    }

    if (!virtual_midi_output_) {
        virtual_midi_output_ = std::make_unique<libremidi::midi_out>(libremidi::output_configuration{}, libremidi::midi2::out_default_configuration());
        if (auto err = virtual_midi_output_->open_virtual_port(virtual_output_name_); err != stdx::error{}) {
            auto msg = err.message();
            std::cerr << "Error opening virtual output port: " << std::string(msg.data(), msg.size()) << std::endl;
            virtual_midi_output_.reset();
        }
    }
}

void MidiDeviceManager::close_virtual_ports_locked() {
    if (virtual_midi_input_) {
        virtual_midi_input_->close_port();
        virtual_midi_input_.reset();
    }
    if (virtual_midi_output_) {
        virtual_midi_output_->close_port();
        virtual_midi_output_.reset();
    }
}

void MidiDeviceManager::reopen_virtual_ports_locked() {
    if (!virtual_ports_enabled_) {
        return;
    }
    close_virtual_ports_locked();
    open_virtual_ports_locked();
}

void MidiDeviceManager::notify_ump_listeners(umppi::UmpWordSpan words, uint64_t timestamp_ns) {
    std::vector<UmpListener> listeners;
    {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        listeners = ump_listeners_;
    }

    if (listeners.empty()) {
        return;
    }

    for (const auto& listener : listeners) {
        if (listener) {
            listener(words, timestamp_ns);
        }
    }
}

std::string MidiDeviceManager::format_ump_packet(const libremidi::ump& packet) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    size_t words = packet.size();
    if (words == 0) {
        words = 1;
    }
    for (size_t i = 0; i < words; ++i) {
        oss << "0x" << std::setw(8) << packet.data[i];
        if (i + 1 < words) {
            oss << ' ';
        }
    }
    return oss.str();
}

void MidiDeviceManager::log_virtual_event(const std::string& message, VirtualPortDirection direction) {
    if (log_callback_) {
        log_callback_(message, direction);
    }
}

bool MidiDeviceManager::extract_note_event(const libremidi::ump& packet, int& note, int& velocity, bool& is_pressed) const {
    const uint32_t word0 = packet.data[0];
    const uint32_t word1 = packet.data[1];
    const uint8_t message_type = (word0 >> 28) & 0xF;
    const uint8_t status = (word0 >> 16) & 0xFF;

    switch (message_type) {
    case 0x4: {
        if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) {
            note = (word0 >> 8) & 0x7F;
            uint16_t velocity16 = static_cast<uint16_t>((word1 >> 16) & 0xFFFF);
            velocity = static_cast<int>(velocity16 >> 9);
            if ((status & 0xF0) == 0x90 && velocity > 0) {
                is_pressed = true;
            } else {
                is_pressed = false;
            }
            return true;
        }
        break;
    }
    case 0x2: {
        if ((status & 0xF0) == 0x90 || (status & 0xF0) == 0x80) {
            note = (word0 >> 8) & 0x7F;
            uint8_t vel7 = static_cast<uint8_t>(word0 & 0xFF);
            if ((status & 0xF0) == 0x90 && vel7 > 0) {
                is_pressed = true;
                velocity = vel7;
            } else {
                is_pressed = false;
                velocity = vel7;
            }
            return true;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

} // namespace ci_tool
