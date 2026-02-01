#include "keyboard_controller.h"
#include <iostream>
#include <algorithm>
#include <libremidi/ump.hpp>
#include <umppi/details/UmpFactory.hpp>
#include <umppi/details/Common.hpp>

namespace {
inline uint8_t getByteFromUmp64(uint64_t ump_data, int index) {
    return static_cast<uint8_t>((ump_data >> ((7 - index) * 8)) & 0xFF);
}
}

KeyboardController::KeyboardController(midicci::keyboard::MessageLogger* logger) 
    : logger_(logger) {
    resetMidiConnections();
}

KeyboardController::~KeyboardController() {
    if (initialized) {
        allNotesOff();
        if (midiIn && midiIn->is_port_open()) {
            midiIn->close_port();
        }
        if (midiOut && midiOut->is_port_open()) {
            midiOut->close_port();
        }
        if (midiCIManager) {
            midiCIManager->shutdown();
        }
    }
}

bool KeyboardController::resetMidiConnections() {
    try {
        // Shutdown existing MIDI-CI manager if it exists
        if (midiCIManager) {
            std::cout << "[RESET] Shutting down existing MIDI-CI manager" << std::endl;
            midiCIManager->shutdown();
            midiCIManager.reset();
        }
        
        // Clear any cached SysEx tracking to avoid stale feedback detection
        recentOutgoingSysEx.clear();
        
        // Create observer with UMP/MIDI 2.0 configuration for device detection
        libremidi::observer_configuration obsConf;
        obsConf.track_hardware = true;   // Track hardware MIDI devices
        obsConf.track_virtual = true;    // Track virtual/software MIDI devices  
        obsConf.track_any = true;        // Track any other types of devices
        obsConf.notify_in_constructor = true;  // Get existing ports immediately
        
        // Add callbacks for device hotplug detection
        obsConf.input_added = [](const libremidi::input_port& port) {
            std::cout << "MIDI Input device connected: " << port.port_name << std::endl;
        };
        obsConf.input_removed = [](const libremidi::input_port& port) {
            std::cout << "MIDI Input device disconnected: " << port.port_name << std::endl;
        };
        obsConf.output_added = [](const libremidi::output_port& port) {
            std::cout << "MIDI Output device connected: " << port.port_name << std::endl;
        };
        obsConf.output_removed = [](const libremidi::output_port& port) {
            std::cout << "MIDI Output device disconnected: " << port.port_name << std::endl;
        };
        
        // Use MIDI 2.0/UMP observer configuration
        observer = std::make_unique<libremidi::observer>(obsConf, libremidi::midi2::observer_default_configuration());
        
        // Create MIDI input with UMP callback configuration
        libremidi::ump_input_configuration inConf {
            .on_message = [this](libremidi::ump&& packet) {
                onMidiInput(std::move(packet));
            },
            .ignore_sysex = false
        };

        midiIn = std::make_unique<libremidi::midi_in>(inConf, libremidi::midi2::in_default_configuration());
        
        // Create MIDI output with UMP configuration
        libremidi::output_configuration outConf;
        midiOut = std::make_unique<libremidi::midi_out>(outConf, libremidi::midi2::out_default_configuration());
        
        
        // Initialize MIDI-CI
        initializeMidiCI();
        
        initialized = true;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "MIDI initialization failed: " << e.what() << std::endl;
        return false;
    }
}

std::vector<std::pair<std::string, std::string>> KeyboardController::getInputDevices() {
    std::vector<std::pair<std::string, std::string>> devices;
    
    try {
        if (!observer) {
            std::cerr << "Observer not initialized" << std::endl;
            return devices;
        }
        
        auto ports = observer->get_input_ports();
        for (size_t i = 0; i < ports.size(); i++) {
            std::string id = std::to_string(i);
            std::string name = ports[i].port_name;
            // Device now supports UMP/MIDI 2.0 by default with the UMP backend
            devices.emplace_back(id, name);
        }
        
        std::cout << "Found " << devices.size() << " input devices" << std::endl;
        for (const auto& device : devices) {
            std::cout << "  ID: " << device.first << " - " << device.second << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting input devices: " << e.what() << std::endl;
    }
    
    return devices;
}

std::vector<std::pair<std::string, std::string>> KeyboardController::getOutputDevices() {
    std::vector<std::pair<std::string, std::string>> devices;
    
    try {
        if (!observer) {
            std::cerr << "Observer not initialized" << std::endl;
            return devices;
        }
        
        auto ports = observer->get_output_ports();
        for (size_t i = 0; i < ports.size(); i++) {
            std::string id = std::to_string(i);
            std::string name = ports[i].port_name;
            // Device now supports UMP/MIDI 2.0 by default with the UMP backend
            devices.emplace_back(id, name);
        }
        
        std::cout << "Found " << devices.size() << " output devices" << std::endl;
        for (const auto& device : devices) {
            std::cout << "  ID: " << device.first << " - " << device.second << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error getting output devices: " << e.what() << std::endl;
    }
    
    return devices;
}

bool KeyboardController::selectInputDevice(const std::string& deviceId) {
    try {
        if (midiIn && midiIn->is_port_open()) {
            midiIn->close_port();
            updateUIConnectionState();
        }

        if (deviceId.empty()) {
            currentInputDeviceId = "";
            updateUIConnectionState();
            return true;
        }

        auto ports = observer->get_input_ports();
        size_t portIndex = std::stoul(deviceId);

        if (portIndex < ports.size()) {
            midiIn->open_port(ports[portIndex]);
            currentInputDeviceId = deviceId;

            // Reinitialize MIDI-CI when we have a valid MIDI pair
            if (hasValidMidiPair()) {
                initializeMidiCI();
            }

            updateUIConnectionState();
            checkAndAutoConnect();

            return true;
        } else {
            return false;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

bool KeyboardController::selectOutputDevice(const std::string& deviceId) {
    try {
        if (midiOut && midiOut->is_port_open()) {
            midiOut->close_port();
            updateUIConnectionState();
        }

        if (deviceId.empty()) {
            currentOutputDeviceId = "";
            updateUIConnectionState();
            return true;
        }

        auto ports = observer->get_output_ports();
        size_t portIndex = std::stoul(deviceId);

        if (portIndex < ports.size()) {
            midiOut->open_port(ports[portIndex]);
            currentOutputDeviceId = deviceId;
            updateUIConnectionState();

            // Reinitialize MIDI-CI when we have a valid MIDI pair
            if (hasValidMidiPair()) {
                initializeMidiCI();
            }

            checkAndAutoConnect();

            return true;
        } else {
            return false;
        }
    } catch (const std::exception& e) {
        return false;
    }
}

void KeyboardController::refreshDevices() {
    std::cout << "Refreshing MIDI devices..." << std::endl;
    // The observer automatically refreshes device lists
    // We just need to call the getter functions to update
    getInputDevices();
    getOutputDevices();
}

void KeyboardController::noteOn(int note, int velocity) {
    if (!initialized) return;
    
    try {
        // Send MIDI 2.0 UMP note on message
        libremidi::ump noteOnPacket = createUmpNoteOn(0, note, velocity);
        dispatch_outgoing_packet(noteOnPacket);
    } catch (const std::exception& e) {
        std::cerr << "Error sending note on: " << e.what() << std::endl;
    }
}

void KeyboardController::noteOff(int note) {
    if (!initialized) return;
    
    try {
        // Send MIDI 2.0 UMP note off message
        libremidi::ump noteOffPacket = createUmpNoteOff(0, note);
        dispatch_outgoing_packet(noteOffPacket);
    } catch (const std::exception& e) {
        std::cerr << "Error sending note off: " << e.what() << std::endl;
    }
}

void KeyboardController::allNotesOff() {
    if (!initialized) return;
    
    try {
        // Send all notes off message
        for (int note = 0; note < 128; note++) {
            noteOff(note);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error sending all notes off: " << e.what() << std::endl;
    }
}

void KeyboardController::onMidiInput(libremidi::ump&& packet) {
    uint8_t message_type = (packet.data[0] >> 28) & 0xF;
    if (message_type == 0x3) {
        uint8_t group = (packet.data[0] >> 24) & 0xF;
        uint8_t status = (packet.data[0] >> 20) & 0xF;
        uint8_t number_of_bytes = (packet.data[0] >> 16) & 0xF;

        // Handle multi-packet SysEx reconstruction manually based on UMP SysEx7 status
        switch (status) {
            case 0x0: { // Complete SysEx in one packet
                sysex_buffer_.clear();

                // Extract data bytes from UMP packet
                uint64_t ump_data = (static_cast<uint64_t>(packet.data[0]) << 32) | static_cast<uint64_t>(packet.data[1]);
                for (int i = 0; i < number_of_bytes && i < 6; i++) {
                    uint8_t data_byte = getByteFromUmp64(ump_data, 2 + i);
                    sysex_buffer_.push_back(data_byte);
                }

                // Check if this is one of our own outgoing messages to avoid feedback loop
                // But be more intelligent - only block exact matches, not legitimate responses
                if (recentOutgoingSysEx.find(sysex_buffer_) != recentOutgoingSysEx.end()) {
                    std::cerr << "[SYSEX ERROR] Echoed SysEx received; invalid per MIDI-CI. Ignoring." << std::endl;
                    if (logger_) {
                        logger_->log("ERROR: Echoed SysEx received; ignoring (invalid per MIDI-CI).", midicci::keyboard::MessageDirection::In);
                    }
                    recentOutgoingSysEx.erase(sysex_buffer_); // Remove from tracking set
                    sysex_in_progress_ = false;
                    return;
                } else {
                    // Log the incoming SysEx message
                    if (logger_) {
                        std::string hex_str = "SysEx In: ";
                        for (size_t i = 0; i < sysex_buffer_.size(); ++i) {
                            hex_str += std::format("{:02X} ", sysex_buffer_[i]);
                        }
                        logger_->log(hex_str, midicci::keyboard::MessageDirection::In);
                    }
                    
                    // Check if this might be a legitimate MIDI-CI message (starts with F0 7E ... 0D)
                    if (sysex_buffer_.size() >= 4 && 
                        sysex_buffer_[0] == 0x7E && sysex_buffer_[2] == 0x0D) {
                        std::cout << "[SYSEX INPUT] Processing legitimate MIDI-CI message" << std::endl;
                        processSysExForMidiCI(group, sysex_buffer_);
                    } else {
                        std::cout << "[SYSEX INPUT] Processing SysEx message (not MIDI-CI or not in recent outgoing)" << std::endl;
                        processSysExForMidiCI(group, sysex_buffer_);
                    }
                }
                sysex_in_progress_ = false;
                break;
            }
            case 0x1: { // SysEx start
                sysex_buffer_.clear();
                sysex_in_progress_ = true;

                // Extract data bytes
                uint64_t ump_data = (static_cast<uint64_t>(packet.data[0]) << 32) | static_cast<uint64_t>(packet.data[1]);
                for (int i = 0; i < number_of_bytes && i < 6; i++) {
                    uint8_t data_byte = getByteFromUmp64(ump_data, 2 + i);
                    sysex_buffer_.push_back(data_byte);
                }
                break;
            }
            case 0x2: { // SysEx continue
                if (!sysex_in_progress_) {
                    std::cerr << "[SYSEX ERROR] Continue packet without start" << std::endl;
                    break;
                }

                // Extract data bytes
                uint64_t ump_data = (static_cast<uint64_t>(packet.data[0]) << 32) | static_cast<uint64_t>(packet.data[1]);
                for (int i = 0; i < number_of_bytes && i < 6; i++) {
                    uint8_t data_byte = getByteFromUmp64(ump_data, 2 + i);
                    sysex_buffer_.push_back(data_byte);
                }
                break;
            }
            case 0x3: { // SysEx end
                if (!sysex_in_progress_) {
                    std::cerr << "[SYSEX ERROR] End packet without start" << std::endl;
                    break;
                }

                // Extract final data bytes
                uint64_t ump_data = (static_cast<uint64_t>(packet.data[0]) << 32) | static_cast<uint64_t>(packet.data[1]);
                for (int i = 0; i < number_of_bytes && i < 6; i++) {
                    uint8_t data_byte = getByteFromUmp64(ump_data, 2 + i);
                    sysex_buffer_.push_back(data_byte);
                }

                // Check if this is one of our own outgoing messages to avoid feedback loop
                // But be more intelligent - only block exact matches, not legitimate responses
                if (recentOutgoingSysEx.find(sysex_buffer_) != recentOutgoingSysEx.end()) {
                    std::cerr << "[SYSEX ERROR] Echoed SysEx received (multi-packet); invalid per MIDI-CI. Ignoring." << std::endl;
                    if (logger_) {
                        logger_->log("ERROR: Echoed SysEx received (multi-packet); ignoring (invalid per MIDI-CI).", midicci::keyboard::MessageDirection::In);
                    }
                    recentOutgoingSysEx.erase(sysex_buffer_); // Remove from tracking set
                    sysex_in_progress_ = false;
                    return;
                } else {
                    // Log the incoming SysEx message
                    if (logger_) {
                        std::string hex_str = "SysEx In (multi-packet): ";
                        for (size_t i = 0; i < sysex_buffer_.size(); ++i) {
                            hex_str += std::format("{:02X} ", sysex_buffer_[i]);
                        }
                        logger_->log(hex_str, midicci::keyboard::MessageDirection::In);
                    }
                    
                    // Check if this might be a legitimate MIDI-CI message (starts with F0 7E ... 0D)
                    if (sysex_buffer_.size() >= 4 && 
                        sysex_buffer_[0] == 0x7E && sysex_buffer_[2] == 0x0D) {
                        std::cout << "[SYSEX INPUT] Processing legitimate MIDI-CI message (multi-packet)" << std::endl;
                        processSysExForMidiCI(group, sysex_buffer_);
                    } else {
                        std::cout << "[SYSEX INPUT] Processing SysEx message (not MIDI-CI or not in recent outgoing, multi-packet)" << std::endl;
                        processSysExForMidiCI(group, sysex_buffer_);
                    }
                }
                sysex_in_progress_ = false;
                break;
            }
            default:
                std::cerr << "[SYSEX ERROR] Unknown SysEx7 status: " << (int)status << std::endl;
                break;
        }
        return;
    }

    int note = 0;
    int velocity = 0;
    bool is_pressed = false;
    if (extract_note_event(packet, note, velocity, is_pressed)) {
        if (incoming_note_callback_) {
            incoming_note_callback_(note, velocity, is_pressed);
        }
    }
}

libremidi::ump KeyboardController::createUmpNoteOn(int channel, int note, int velocity) {
    // Create MIDI 2.0 Note On UMP packet
    // Format: [Message Type (4) | Channel (4) | Status (8) | Note (8) | Reserved (8)] [Velocity (16) | Attribute Type (8) | Attribute Data (8)] [0] [0]
    uint32_t word0 = (0x4 << 28) | (channel << 24) | (0x90 << 16) | (note << 8) | 0x00;
    uint32_t word1 = (velocity << 16) | 0x0000; // MIDI 2.0 uses 16-bit velocity in upper 16 bits, attribute type/data in lower 16 bits
    return libremidi::ump(word0, word1, 0, 0);
}

libremidi::ump KeyboardController::createUmpNoteOff(int channel, int note) {
    // Create MIDI 2.0 Note Off UMP packet
    // Format: [Message Type (4) | Channel (4) | Status (8) | Note (8) | Reserved (8)] [Velocity (16) | Attribute Type (8) | Velocity MSB (8)] [0] [0]
    uint32_t word0 = (0x4 << 28) | (channel << 24) | (0x80 << 16) | (note << 8) | 0x00;
    uint32_t word1 = 0; // Zero velocity for note off
    return libremidi::ump(word0, word1, 0, 0);
}


void KeyboardController::sendMidiCIDiscovery() {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->sendDiscovery();
    }
}

std::vector<std::string> KeyboardController::getMidiCIDevices() {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getDiscoveredDevices();
    }
    return {};
}

std::vector<MidiCIDeviceInfo> KeyboardController::getMidiCIDeviceDetails() {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getDiscoveredDeviceDetails();
    }
    return {};
}

MidiCIDeviceInfo* KeyboardController::getMidiCIDeviceByMuid(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getDeviceByMuid(muid);
    }
    return nullptr;
}

bool KeyboardController::isMidiCIInitialized() const {
    return midiCIManager && midiCIManager->isInitialized();
}

uint32_t KeyboardController::getMidiCIMuid() const {
    if (midiCIManager) {
        return midiCIManager->getMuid();
    }
    return 0;
}

std::string KeyboardController::getMidiCIDeviceName() const {
    if (midiCIManager) {
        return midiCIManager->getDeviceName();
    }
    return "";
}

void KeyboardController::setMidiCIDevicesChangedCallback(std::function<void()> callback) {
    midiCIDevicesChangedCallback = callback;
    if (midiCIManager) {
        midiCIManager->setDevicesChangedCallback(callback);
    }
}

// MIDI-CI Property methods - simplified API using PropertyClientFacade
std::optional<std::vector<midicci::commonproperties::MidiCIControl>> KeyboardController::getAllCtrlList(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getAllCtrlList(muid);
    }
    return std::nullopt;
}

std::optional<std::vector<midicci::commonproperties::MidiCIProgram>> KeyboardController::getProgramList(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getProgramList(muid);
    }
    return std::nullopt;
}

std::optional<std::vector<midicci::commonproperties::MidiCIControlMap>> KeyboardController::getCtrlMapList(uint32_t muid, const std::string& ctrlMapId) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        return midiCIManager->getCtrlMapList(muid, ctrlMapId);
    }
    return std::nullopt;
}

void KeyboardController::requestCtrlMapList(uint32_t muid, const std::string& ctrlMapId) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->requestCtrlMapList(muid, ctrlMapId);
    }
}

void KeyboardController::requestAllCtrlList(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->requestAllCtrlList(muid);
    }
}

void KeyboardController::requestProgramList(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->requestProgramList(muid);
    }
}

void KeyboardController::requestSaveState(uint32_t muid) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->requestSaveState(muid);
    }
}

void KeyboardController::sendState(uint32_t muid, const std::string& stateId, const std::vector<uint8_t>& data) {
    if (midiCIManager && midiCIManager->isInitialized()) {
        midiCIManager->sendState(muid, stateId, data);
    }
}

void KeyboardController::setStateSaveCallback(std::function<void(uint32_t, const std::vector<uint8_t>&)> callback) {
    stateSaveCallback = callback;
    if (midiCIManager) {
        midiCIManager->setStateSaveCallback(callback);
    }
}

void KeyboardController::setMidiCIPropertiesChangedCallback(std::function<void(uint32_t, const std::string&, const std::string&)> callback) {
    midiCIPropertiesChangedCallback = callback;
    if (midiCIManager) {
        midiCIManager->setPropertiesChangedCallback(callback);
    }
}


bool KeyboardController::hasValidMidiPair() const {
    return midiIn && midiIn->is_port_open() && midiOut && midiOut->is_port_open();
}

void KeyboardController::setMidiConnectionChangedCallback(std::function<void(bool)> callback) {
    midiConnectionChangedCallback = callback;
}

void KeyboardController::setExternalOutputCallback(std::function<void(const libremidi::ump&)> callback) {
    external_output_callback_ = std::move(callback);
}

void KeyboardController::setIncomingNoteCallback(std::function<void(int, int, bool)> callback) {
    incoming_note_callback_ = std::move(callback);
}

void KeyboardController::initializeMidiCI() {
    try {
        // Ensure any existing manager is properly shut down first
        if (midiCIManager) {
            std::cout << "[MIDI-CI] Reinitializing MIDI-CI manager" << std::endl;
            // Preserve the MUID from the existing manager
            if (local_app_muid == 0) {
                local_app_muid = midiCIManager->getMuid();
            }
            midiCIManager->shutdown();
            midiCIManager.reset();
        }
        
        midiCIManager = std::make_unique<MidiCIManager>(logger_);
        
        // Set up logging callback
        midiCIManager->setLogCallback([](const std::string& message) {
            std::cout << message << std::endl;
        });
        
        // Set up SysEx sender callback BEFORE initialization
        midiCIManager->setSysExSender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
            std::cout << "[SYSEX CALLBACK] External SysEx sender called with " << data.size() << " bytes" << std::endl;
            return sendSysExViaMidi(group, data);
        });
        
        // Initialize the MIDI-CI manager (will now use the SysEx sender)
        if (!midiCIManager->initialize(local_app_muid)) {
            std::cerr << "Failed to initialize MIDI-CI manager" << std::endl;
            midiCIManager.reset();
        } else {
            // Store the MUID if this is the first initialization
            if (local_app_muid == 0) {
                local_app_muid = midiCIManager->getMuid();
            }
            // Set up stored callbacks after successful initialization
            if (midiCIPropertiesChangedCallback) {
                midiCIManager->setPropertiesChangedCallback(midiCIPropertiesChangedCallback);
                std::cout << "[MIDI-CI] Properties changed callback restored after initialization" << std::endl;
            }
            if (midiCIDevicesChangedCallback) {
                midiCIManager->setDevicesChangedCallback(midiCIDevicesChangedCallback);
                std::cout << "[MIDI-CI] Devices changed callback restored after initialization" << std::endl;
            }
            if (stateSaveCallback) {
                midiCIManager->setStateSaveCallback(stateSaveCallback);
                std::cout << "[MIDI-CI] State save callback restored after initialization" << std::endl;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing MIDI-CI: " << e.what() << std::endl;
        midiCIManager.reset();
    }
}

void KeyboardController::processSysExForMidiCI(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    std::cout << "[MIDI-CI CHECK] Processing SysEx for MIDI-CI, size: " << sysex_data.size() << std::endl;
    
    if (midiCIManager && midiCIManager->isInitialized()) {
        // Check if this is a MIDI-CI message (starts with 0x7E for Universal Non-Real Time)
        if (!sysex_data.empty() && sysex_data.size() > 1 && sysex_data[0] == 0x7E && sysex_data[2] == 0xD) {
            std::cout << "[MIDI-CI DETECTED] Universal Non-Real Time SysEx (0x7E)" << std::endl;
            
            if (sysex_data.size() > 3) {
                std::cout << "[MIDI-CI INFO] Device ID: 0x" << std::hex << (int)sysex_data[2] << std::dec;
                if (sysex_data.size() > 4) {
                    std::cout << ", Sub-ID1: 0x" << std::hex << (int)sysex_data[3] << std::dec;
                    if (sysex_data[3] == 0x0D) {
                        std::cout << " (MIDI-CI)";
                    }
                }
                std::cout << std::endl;
            }
            
            // Strip F0 start and F7 end bytes if present (MIDI 1.0),
            // but UMP SysEx7 does not include them so this usually keeps the buffer unchanged.
            std::vector<uint8_t> payload_data;
            if (sysex_data.size() >= 2) {
                // Skip F0 at start, and F7 at end if present
                size_t start_idx = (sysex_data[0] == 0xF0) ? 1 : 0;
                size_t end_idx = sysex_data.size();
                if (end_idx > 0 && sysex_data[end_idx - 1] == 0xF7) {
                    end_idx--;
                }
                
                if (end_idx > start_idx) {
                    payload_data.assign(sysex_data.begin() + start_idx, sysex_data.begin() + end_idx);
                    
                    std::cout << "[MIDI-CI PAYLOAD] Stripped F0/F7, payload size: " << payload_data.size() << std::endl;
                    std::cout << "[MIDI-CI PAYLOAD] Data: ";
                    for (size_t i = 0; i < payload_data.size(); i++) {
                        std::cout << std::hex << "0x" << (int)payload_data[i] << " ";
                    }
                    std::cout << std::dec << std::endl;
                    
                    // UMP-aware processing (preserve group)
                    midiCIManager->processUmpSysEx(group, payload_data);
                } else {
                    std::cout << "[MIDI-CI ERROR] Invalid SysEx payload after stripping F0/F7" << std::endl;
                }
            }
        } else {
            std::cout << "[MIDI-CI SKIP] Not a MIDI-CI message (not 0xF0 0x7E)" << std::endl;
        }
    } else {
        std::cout << "[MIDI-CI SKIP] MIDI-CI Manager not initialized" << std::endl;
    }
}

bool KeyboardController::sendSysExViaMidi(uint8_t group, const std::vector<uint8_t>& data) {
    if (!initialized) {
        return false;
    }
    
    try {
        // Log the outgoing SysEx message
        if (logger_) {
            std::string hex_str = "SysEx Out: ";
            for (size_t i = 0; i < data.size(); ++i) {
                hex_str += std::format("{:02X} ", data[i]);
            }
            logger_->log(hex_str, midicci::keyboard::MessageDirection::Out);
        }
        
        // Track this outgoing message to avoid processing it as input
        recentOutgoingSysEx.insert(data);
        // Keep only recent messages to prevent memory growth
        if (recentOutgoingSysEx.size() > 10) {
            auto it = recentOutgoingSysEx.begin();
            recentOutgoingSysEx.erase(it);
        }

        // Convert SysEx to UMP SYSEX7 packets and send
        try {
            umppi::UmpFactory::sysex7Process(group, data, [this](const umppi::Ump& ump) {
                libremidi::ump packet(ump.int1, ump.int2, 0, 0);
                dispatch_outgoing_packet(packet);
            });
            std::cout << "[SYSEX SEND] UMP SYSEX7 packets sent successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[SYSEX SEND] Failed to process SysEx: " << e.what() << std::endl;
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error sending SysEx via UMP: " << e.what() << std::endl;
        return false;
    }
}

namespace {
int clamp_group(int group) {
    return std::clamp(group, 0, 15);
}
} // namespace

void KeyboardController::sendControlChange(int channel, int controller, uint32_t value, int group) {
    if (!initialized) return;

    try {
        auto cc = umppi::UmpFactory::midi2CC(clamp_group(group), channel, controller, value);
        libremidi::ump packet(cc >> 32, cc & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] CC Ch:" << channel << " CC:" << controller << " Val:" << value << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending control change: " << e.what() << std::endl;
    }
}

void KeyboardController::sendRPN(int channel, int msb, int lsb, uint32_t value, int group) {
    if (!initialized) return;

    try {
        auto rpn = umppi::UmpFactory::midi2RPN(clamp_group(group), channel, msb, lsb, value);
        libremidi::ump packet(rpn >> 32, rpn & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] RPN Ch:" << channel << " MSB:" << msb << " LSB:" << lsb << " Val:" << value << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending RPN: " << e.what() << std::endl;
    }
}

void KeyboardController::sendNRPN(int channel, int msb, int lsb, uint32_t value, int group) {
    if (!initialized) return;

    try {
        auto nrpn = umppi::UmpFactory::midi2NRPN(clamp_group(group), channel, msb, lsb, value);
        libremidi::ump packet(nrpn >> 32, nrpn & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] NRPN Ch:" << channel << " MSB:" << msb << " LSB:" << lsb << " Val:" << value << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending NRPN: " << e.what() << std::endl;
    }
}

void KeyboardController::sendPerNoteControlChange(int channel, int note, int controller, uint32_t value, int group) {
    if (!initialized) return;

    try {
        auto pnac = umppi::UmpFactory::midi2PerNoteACC(clamp_group(group), channel, note, controller, value);
        libremidi::ump packet(pnac >> 32, pnac & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] Per-Note CC Ch:" << channel << " Note:" << note << " CC:" << controller << " Val:" << value << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending per-note control change: " << e.what() << std::endl;
    }
}

void KeyboardController::sendPerNoteAftertouch(int channel, int note, uint32_t value, int group) {
    if (!initialized) return;

    try {
        auto paf = umppi::UmpFactory::midi2PAf(clamp_group(group), channel, note, value);
        libremidi::ump packet(paf >> 32, paf & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] Per-Note AC Ch:" << channel << " Note:" << note << " Val:" << value << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending per-note aftertouch: " << e.what() << std::endl;
    }
}

void KeyboardController::sendProgramChange(int channel, uint8_t program, uint8_t bankMSB, uint8_t bankLSB, int group) {
    if (!initialized) return;

    try {
        auto pc = umppi::UmpFactory::midi2Program(clamp_group(group), channel, umppi::MidiProgramChangeOptions::BANK_VALID, program, bankMSB, bankLSB);
        libremidi::ump packet(pc >> 32, pc & 0xFFFFFFFF, 0, 0);
        dispatch_outgoing_packet(packet);

        std::cout << "[MIDI OUT] Program Change Ch:" << channel << " Program:" << (int)program
                  << " Bank MSB:" << (int)bankMSB << " Bank LSB:" << (int)bankLSB << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error sending program change: " << e.what() << std::endl;
    }
}

void KeyboardController::updateUIConnectionState() {
    bool currentConnectionState = hasValidMidiPair();

    if (currentConnectionState != previousConnectionState) {
        previousConnectionState = currentConnectionState;

        if (!currentConnectionState && midiCIManager) {
            std::cout << "[MIDI-CI] Shutting down MIDI-CI manager due to invalid MIDI pair" << std::endl;
            midiCIManager->shutdown();
            midiCIManager.reset();
        }

        if (midiConnectionChangedCallback) {
            midiConnectionChangedCallback(currentConnectionState);
        }

        std::cout << "MIDI connection pair state changed: " << (currentConnectionState ? "CONNECTED" : "DISCONNECTED") << std::endl;
    }
}

std::string KeyboardController::normalizeDeviceName(const std::string& deviceName) {
    if (deviceName.length() >= 3 && deviceName.substr(deviceName.length() - 3) == " In")
        return deviceName.substr(0, deviceName.length() - 3);
    if (deviceName.length() >= 4 && deviceName.substr(deviceName.length() - 4) == " Out")
        return deviceName.substr(0, deviceName.length() - 4);
    return deviceName;
}

void KeyboardController::checkAndAutoConnect() {
    if (!hasValidMidiPair()) {
        return;
    }

    if (!observer) {
        return;
    }

    try {
        std::string inputDeviceName;
        std::string outputDeviceName;

        auto inputPorts = observer->get_input_ports();
        auto outputPorts = observer->get_output_ports();

        size_t inputIndex = currentInputDeviceId.empty() ? SIZE_MAX : std::stoul(currentInputDeviceId);
        size_t outputIndex = currentOutputDeviceId.empty() ? SIZE_MAX : std::stoul(currentOutputDeviceId);

        if (inputIndex < inputPorts.size()) {
            inputDeviceName = inputPorts[inputIndex].port_name;
        }

        if (outputIndex < outputPorts.size()) {
            outputDeviceName = outputPorts[outputIndex].port_name;
        }

        if (inputDeviceName.empty() || outputDeviceName.empty()) {
            return;
        }

        std::string normalizedInput = normalizeDeviceName(inputDeviceName);
        std::string normalizedOutput = normalizeDeviceName(outputDeviceName);

        if (normalizedInput == normalizedOutput) {
            std::cout << "Auto-connecting: matched devices '" << inputDeviceName
                      << "' and '" << outputDeviceName << "'" << std::endl;

            if (midiCIManager && midiCIManager->isInitialized()) {
                midiCIManager->sendDiscovery();
                std::cout << "Automatically sending discovery inquiry" << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in auto-connect check: " << e.what() << std::endl;
    }
}

void KeyboardController::dispatch_outgoing_packet(const libremidi::ump& packet) {
    try {
        if (midiOut) {
            midiOut->send_ump(packet);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error sending packet to MIDI out: " << e.what() << std::endl;
    }
    if (external_output_callback_) {
        external_output_callback_(packet);
    }
}

bool KeyboardController::extract_note_event(const libremidi::ump& packet, int& note, int& velocity, bool& is_pressed) const {
    const uint32_t word0 = packet.data[0];
    const uint32_t word1 = packet.data[1];
    const uint8_t message_type = (word0 >> 28) & 0xF;
    const uint8_t status = (word0 >> 16) & 0xFF;

    switch (message_type) {
    case 0x4: { // MIDI 2.0 channel voice
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
    case 0x2: { // MIDI 1.0 channel voice
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
