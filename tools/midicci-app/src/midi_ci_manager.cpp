#include "midi_ci_manager.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include <umppi/details/UmpTranslator.hpp>
#include <midicci/details/Message.hpp>
#include <midicci/details/PropertyValue.hpp>

using namespace midicci::commonproperties;

namespace {
constexpr auto PROPERTY_REQUEST_TIMEOUT = std::chrono::seconds(3);
}

MidiCIManager::MidiCIManager(midicci::keyboard::MessageLogger* logger) 
    : muid_(0), initialized_(false), logger_(logger), instrumentation_call_counter_(0) {
}

MidiCIManager::~MidiCIManager() {
    shutdown();
}

bool MidiCIManager::initialize(uint32_t muid) {
    try {
        if (initialized_) {
            std::cout << "[MIDI-CI] MIDI-CI Manager already initialized" << std::endl;
            return true;
        }
        
        // Generate MUID if not provided
        if (muid == 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
            muid_ = dis(gen) & 0x7F7F7F7F;
        } else {
            muid_ = muid;
        }
        
        // Create device configuration
        setupDeviceConfiguration();
        
        // Create MIDI-CI device with logger callback that handles both strings and Messages
        device_ = std::make_unique<midicci::MidiCIDevice>(
            muid_, 
            *config_,
            [this](const midicci::LogData& log_data) {
                if (logger_) {
                    if (log_data.hasMessage()) {
                        // Log structured MIDI-CI message with proper MUID extraction
                        const auto& message = log_data.getMessage();
                        auto direction = log_data.is_outgoing ?
                            midicci::keyboard::MessageDirection::Out :
                            midicci::keyboard::MessageDirection::In;
                        logger_->log_midi_ci_message(message, direction);
                    } else {
                        // Log plain string message
                        auto direction = log_data.is_outgoing ?
                            midicci::keyboard::MessageDirection::Out :
                            midicci::keyboard::MessageDirection::In;
                        logger_->log(log_data.getString(), direction);
                    }
                }
            }
        );
        
        // Setup callbacks
        setupCallbacks();
        
        // Set up SysEx sender if already provided
        if (sysex_sender_) {
            device_->setSysexSender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
                if (sysex_sender_) {
                    return sysex_sender_(group, data);
                }
                return false;
            });
        }
        
        initialized_ = true;
        std::cout << "[MIDI-CI] MIDI-CI Manager initialized with MUID: 0x" 
                  << std::hex << muid_ << std::dec << " (" << muid_ << ")" << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to initialize MIDI-CI Manager: " << e.what() << std::endl;
        return false;
    }
}

void MidiCIManager::shutdown() {
    if (!initialized_) return;
    
    try {
        // Clear all state before shutting down
        clearDiscoveredDevices();
        
        device_.reset();
        config_.reset();
        initialized_ = false;
        std::cout << "[MIDI-CI] MIDI-CI Manager shutdown complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Error during MIDI-CI Manager shutdown: " << e.what() << std::endl;
    }
}

void MidiCIManager::processMidi1SysEx(const std::vector<uint8_t>& sysex_data) {
    if (!initialized_ || !device_) return;
    
    std::cout << "[MIDICCI] Processing MIDI 1.0 SysEx, size: " << sysex_data.size() << std::endl;
    std::cout << "[MIDICCI] SysEx data: ";
    for (size_t i = 0; i < std::min(sysex_data.size(), size_t(16)); i++) {
        std::cout << std::hex << "0x" << (int)sysex_data[i] << " ";
    }
    if (sysex_data.size() > 16) std::cout << "... (truncated)";
    std::cout << std::dec << std::endl;
    
    try {
        // Process MIDI 1.0 SysEx data through MIDI-CI device
        device_->processInput(0, sysex_data); // Use group 0 for MIDI 1.0
        std::cout << "[MIDICCI] SysEx processed successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Error processing MIDI 1.0 SysEx: " << e.what() << std::endl;
    }
}

// Claude Code was stupid - it somehow did not use this function to process UMP input
// while this app is totally based on UMP...
// We may fix this inconsistency, but I'm afraid that Claude Code is not clever enough to handle this...
void MidiCIManager::processUmpSysEx(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    if (!initialized_ || !device_) return;
    
    try {
        // Process UMP SysEx data through MIDI-CI device
        device_->processInput(group, sysex_data);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Error processing UMP SysEx: " << e.what() << std::endl;
    }
}

void MidiCIManager::sendDiscovery() {
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot send discovery - MIDI-CI Manager not initialized" << std::endl;
        return;
    }
    
    std::cout << "[DISCOVERY SEND] Sending MIDI-CI Discovery inquiry..." << std::endl;
    
    try {
        device_->sendDiscovery();
        log("Discovery inquiry sent", true); // outgoing message
        std::cout << "[DISCOVERY SEND] Discovery inquiry sent successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Error sending discovery: " << e.what() << std::endl;
    }
}

std::vector<std::string> MidiCIManager::getDiscoveredDevices() const {
    std::vector<std::string> devices;
    
    for (const auto& device_info : discovered_devices_) {
        devices.push_back(device_info.getDisplayName());
    }
    
    return devices;
}

std::vector<MidiCIDeviceInfo> MidiCIManager::getDiscoveredDeviceDetails() const {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    return discovered_devices_;
}

MidiCIDeviceInfo* MidiCIManager::getDeviceByMuid(uint32_t muid) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    for (auto& device_info : discovered_devices_) {
        if (device_info.muid == muid) {
            return &device_info;
        }
    }
    return nullptr;
}

void MidiCIManager::setSysExSender(SysExSender sender) {
    sysex_sender_ = sender;
    
    if (initialized_ && device_ && sender) {
        // Set up the CI output sender for the device
        device_->setSysexSender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
            if (sysex_sender_) {
                return sysex_sender_(group, data);
            }
            return false;
        });
    }
}

void MidiCIManager::setLogCallback(LogCallback callback) {
    log_callback_ = callback;
}

void MidiCIManager::setDevicesChangedCallback(DevicesChangedCallback callback) {
    devices_changed_callback_ = callback;
}

void MidiCIManager::setPropertiesChangedCallback(std::function<void(uint32_t, const std::string&, const std::string&)> callback) {
    properties_changed_callback_ = callback;
}

void MidiCIManager::setStateSaveCallback(std::function<void(uint32_t, const std::vector<uint8_t>&)> callback) {
    state_save_callback_ = callback;
}

uint32_t MidiCIManager::getMuid() const {
    return muid_;
}

std::string MidiCIManager::getDeviceName() const {
    if (!initialized_ || !config_) return "";
    return config_->device_info.model;
}

bool MidiCIManager::isInitialized() const {
    return initialized_;
}

void MidiCIManager::setupDeviceConfiguration() {
    config_ = std::make_unique<midicci::MidiCIDeviceConfiguration>();
    
    // Set up basic device information using the correct fields
    //
    // Claude Code is kind of stupid here - it takes some example data as midicci default.
    // We may fix this later, maybe without Claude Code.
    config_->device_info.manufacturer_id = 0x654321;  // Use midicci default for now
    config_->device_info.family_id = 0x4321;
    config_->device_info.model_id = 0x765;
    config_->device_info.version_id = 0x00000001;
    config_->device_info.manufacturer = "atsushieno";
    config_->device_info.family = "UMP";
    config_->device_info.model = "UMP Keyboard";
    config_->device_info.version = "1.0";
    config_->device_info.serial_number = "UMP-KB-001";
    
    // Enable basic capabilities
    config_->capability_inquiry_supported = static_cast<uint8_t>(midicci::MidiCISupportedCategories::THREE_P);
    config_->auto_send_endpoint_inquiry = true;
    config_->auto_send_profile_inquiry = true;
    config_->auto_send_property_exchange_capabilities_inquiry = true;
    config_->auto_send_process_inquiry = true;
    config_->auto_send_get_resource_list = true;
    config_->auto_send_get_device_info = true;
    
    // Add basic General MIDI profile
    std::vector<uint8_t> gm_profile_data{0x7E, 0x00, 0x00, 0x00, 0x01}; // General MIDI Level 1
    midicci::MidiCIProfileId gm_profile_id(gm_profile_data);
    midicci::MidiCIProfile gm_profile(gm_profile_id, 0, 0, false, 16); // group 0, address 0, not enabled initially, 16 channels
    config_->local_profiles.push_back(gm_profile);
}

void MidiCIManager::setupCallbacks() {
    if (!device_) return;

    device_->setPropertyChunkCallback([this](uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& header) {
        std::string request_key;
        if (auto mapped = this->getRequestKeyForId(source_muid, request_id)) {
            request_key = *mapped;
        }

        if (request_key.empty()) {
            if (!this->device_) {
                return;
            }
            auto connection = this->device_->getConnection(source_muid);
            if (!connection) {
                return;
            }
            auto* rules = connection->getPropertyClientFacade().getPropertyRules();
            if (!rules) {
                return;
            }
            if (header.empty()) {
                return;
            }
            try {
                std::string property_id = rules->getPropertyIdForHeader(header);
                if (property_id.empty()) {
                    return;
                }
                std::string res_id = rules->getResIdForHeader(header);
                request_key = res_id.empty() ? property_id : (property_id + ":" + res_id);
            } catch (const std::exception& e) {
                std::cerr << "[MIDI-CI ERROR] Failed to parse property chunk header: " << e.what() << std::endl;
                return;
            }
        }

        if (!request_key.empty()) {
            this->extendPendingPropertyRequest(source_muid, request_key);
        }
    });
    
    // Set up message callback for outgoing messages
    device_->setMessageCallback([this](const midicci::Message& message) {
        std::string msgTypeName;
        switch(message.getType()) {
            case midicci::MessageType::DiscoveryInquiry: msgTypeName = "DiscoveryInquiry"; break;
            case midicci::MessageType::DiscoveryReply: msgTypeName = "DiscoveryReply"; break;
            case midicci::MessageType::GetPropertyData: msgTypeName = "GetPropertyData"; break;
            case midicci::MessageType::GetPropertyDataReply: msgTypeName = "GetPropertyDataReply"; break;
            default: msgTypeName = "Unknown(" + std::to_string(static_cast<int>(message.getType())) + ")"; break;
        }
        std::cout << "[MIDI-CI SENT] " << msgTypeName << " to MUID: 0x" << std::hex << message.getDestinationMuid() << std::dec << std::endl;
    });
    
    // Set up message received callback
    device_->setMessageReceivedCallback([this](const midicci::Message& message) {
        std::cout << "[UMP-KEYBOARD RECV] Message type: " << static_cast<int>(message.getType()) << std::endl;
        
        // Handle Endpoint Reply messages to populate device combobox
        if (message.getType() == midicci::MessageType::EndpointReply) {
            std::cout << "[ENDPOINT REPLY] Processing endpoint reply message" << std::endl;
            
            try {
                const auto* endpoint_reply = dynamic_cast<const midicci::EndpointReply*>(&message);
                if (endpoint_reply) {
                    uint32_t source_muid = endpoint_reply->getSourceMuid();

                    std::cout << "[ENDPOINT REPLY] Source MUID: 0x" << std::hex << source_muid << std::dec << " (" << source_muid << ")" << std::endl;

                    // Extract product_instance_id from EndpointReply data
                    const auto& data = endpoint_reply->getData();
                    std::string product_instance_id;
                    if (!data.empty()) {
                        product_instance_id = std::string(data.begin(), data.end());
                        std::cout << "[ENDPOINT REPLY] Product Instance ID: " << product_instance_id << std::endl;
                    }

                    // Find the device and update its information
                    bool found = false;
                    for (auto& device : discovered_devices_) {
                        if (device.muid == source_muid) {
                            std::cout << "[ENDPOINT REPLY] Marking device MUID 0x" << std::hex << source_muid << std::dec << " as endpoint ready" << std::endl;
                            device.endpoint_ready = true;

                            // Try to get DeviceInfo from the connection
                            if (device_) {
                                auto connection = device_->getConnection(source_muid);
                                if (connection) {
                                    const auto* device_info = connection->getDeviceInfo();
                                    if (device_info) {
                                        device.manufacturer = device_info->manufacturer;
                                        device.model = device_info->model;
                                        device.version = device_info->version;
                                        std::cout << "[ENDPOINT REPLY] Updated device info - Manufacturer: " << device.manufacturer
                                                  << ", Model: " << device.model << ", Version: " << device.version << std::endl;
                                    }
                                }
                            }

                            // If DeviceInfo wasn't available, at least use product_instance_id as model name
                            if (device.model == "MIDI-CI Device" && !product_instance_id.empty()) {
                                device.model = product_instance_id;
                                std::cout << "[ENDPOINT REPLY] Using product_instance_id as model: " << product_instance_id << std::endl;
                            }

                            found = true;
                            break;
                        }
                    }
                    
                    if (found) {
                        std::cout << "[ENDPOINT REPLY] Device MUID 0x" << std::hex << source_muid << std::dec << " ready for UI combobox" << std::endl;
                        
                        // Notify about device list change to update UI combobox
                        if (devices_changed_callback_) {
                            std::cout << "[ENDPOINT REPLY] Calling devices changed callback to update combobox" << std::endl;
                            devices_changed_callback_();
                        } else {
                            std::cout << "[ENDPOINT REPLY] No devices changed callback set" << std::endl;
                        }
                    } else {
                        std::cout << "[ENDPOINT REPLY] WARNING: EndpointReply received for unknown MUID 0x" << std::hex << source_muid << std::dec << std::endl;
                    }
                } else {
                    std::cout << "[ENDPOINT REPLY ERROR] Failed to cast to EndpointReply" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[MIDI-CI ERROR] Error processing endpoint reply: " << e.what() << std::endl;
            }
        }
        
        // Handle Discovery Reply messages to populate device list
        if (message.getType() == midicci::MessageType::DiscoveryReply) {
            std::cout << "[DISCOVERY REPLY] Processing discovery reply message" << std::endl;
            
            // Try to cast to DiscoveryReply to get device details
            try {
                const auto* discovery_reply = dynamic_cast<const midicci::DiscoveryReply*>(&message);
                if (discovery_reply) {
                    uint32_t source_muid = discovery_reply->getSourceMuid();
                    const auto& device_details = discovery_reply->getDeviceDetails();
                    
                    std::cout << "[DISCOVERY REPLY] Source MUID: 0x" << std::hex << source_muid << std::dec << " (" << source_muid << ")" << std::endl;
                    std::cout << "[DISCOVERY REPLY] Device details - Manufacturer: 0x" << std::hex << device_details.manufacturer << std::dec 
                              << ", Family: 0x" << std::hex << device_details.family << std::dec
                              << ", Model: 0x" << std::hex << device_details.modelNumber << std::dec << std::endl;
                    
                    // Check if we already have this device
                    bool found = false;
                    for (auto& existing : discovered_devices_) {
                        if (existing.muid == source_muid) {
                            std::cout << "[DISCOVERY REPLY] Device already exists in list" << std::endl;
                            found = true;
                            break;
                        }
                    }
                    
                    if (!found) {
                        std::cout << "[DISCOVERY REPLY] Adding new device to list" << std::endl;
                        
                        // Add new device to our list
                        MidiCIDeviceInfo new_device(
                            source_muid,
                            "MIDI-CI Device", // placeholder name
                            "Unknown", // Will be populated later if available
                            "MIDI-CI Device", // placeholder model
                            "1.0", // placeholder version
                            0, // features placeholder
                            4096 // default sysex size
                        );
                        discovered_devices_.push_back(new_device);
                        
                        log("New MIDI-CI device discovered: MUID 0x" + std::to_string(source_muid), false); // incoming discovery
                        std::cout << "[DISCOVERY REPLY] Device added, total devices: " << discovered_devices_.size() << std::endl;
                        
                        // Notify about device list change
                        if (devices_changed_callback_) {
                            std::cout << "[DISCOVERY REPLY] Calling devices changed callback" << std::endl;
                            devices_changed_callback_();
                        } else {
                            std::cout << "[DISCOVERY REPLY] No devices changed callback set" << std::endl;
                        }
                    }
                } else {
                    std::cout << "[DISCOVERY REPLY ERROR] Failed to cast to DiscoveryReply" << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "[MIDI-CI ERROR] Error processing discovery reply: " << e.what() << std::endl;
            }
        }
    });
    
    // Set up connections changed callback
    device_->setConnectionsChangedCallback([this]() {
        log("MIDI-CI Connections changed", false); // incoming change notification
        
        // Defer device access to avoid potential re-entrancy or lock inversion
        std::thread([this]() {
            try {
                auto& connections = device_->getConnections();
                for (const auto& conn_pair : connections) {
                    uint32_t muid = conn_pair.first;
                    std::cout << "[CONNECTIONS CHANGED] Setting up property callbacks for MUID: 0x" << std::hex << muid << std::dec << std::endl;
                    setupPropertyCallbacks(muid);
                }
                if (devices_changed_callback_) {
                    devices_changed_callback_();
                }
            } catch (const std::exception& e) {
                std::cerr << "[MIDI-CI ERROR] connections_changed handling failed: " << e.what() << std::endl;
            }
        }).detach();
    });
}

void MidiCIManager::log(const std::string& message, bool is_outgoing) {
    std::string prefix = is_outgoing ? "[MIDI-CI OUT] " : "[MIDI-CI IN] ";
    std::string full_message = prefix + message;

    // Log to logger if available
    if (logger_) {
        auto direction = is_outgoing ? midicci::keyboard::MessageDirection::Out :
                                      midicci::keyboard::MessageDirection::In;
        logger_->log(full_message, direction);
    }
    
    if (log_callback_) {
        log_callback_(full_message);
    } else {
        std::cout << full_message << std::endl;
    }
}


// Property management methods using PropertyClientFacade for remote device access
std::optional<std::vector<midicci::commonproperties::MidiCIControl>> MidiCIManager::getAllCtrlList(uint32_t muid) {
    instrumentation_log_property_call(muid, "AllCtrlList(read)", "getAllCtrlList");
    
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }
    
    // Get connection to the remote device
    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }
    
    try {
        auto* properties = connection->getPropertyClientFacade().getProperties();
        if (!properties) {
            std::cout << "[PROPERTY ACCESS] No ObservablePropertyList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }
        auto ret = StandardPropertiesExtensions::getAllCtrlList(*properties);
        return ret;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to get AllCtrlList for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::vector<midicci::commonproperties::MidiCIProgram>> MidiCIManager::getProgramList(uint32_t muid) {
    instrumentation_log_property_call(muid, "ProgramList(read)", "getProgramList");
    
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }
    
    // Get connection to the remote device
    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }
    
    try {
        auto* properties = connection->getPropertyClientFacade().getProperties();
        if (!properties) {
            std::cout << "[PROPERTY ACCESS] No ObservablePropertyList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }
        auto ret = StandardPropertiesExtensions::getProgramList(*properties);
        return ret;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to get ProgramList for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::vector<midicci::commonproperties::MidiCIControlMap>> MidiCIManager::getCtrlMapList(uint32_t muid, const std::string& ctrlMapId) {
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }

    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }

    try {
        auto* properties = connection->getPropertyClientFacade().getProperties();
        if (!properties) {
            std::cout << "[PROPERTY ACCESS] No ObservablePropertyList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }
        return StandardPropertiesExtensions::getCtrlMapList(*properties, ctrlMapId);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to get CtrlMapList[" << ctrlMapId << "] for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

void MidiCIManager::requestCtrlMapList(uint32_t muid, const std::string& ctrlMapId) {
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot request CtrlMapList - not initialized" << std::endl;
        return;
    }

    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY REQUEST] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return;
    }

    try {
        std::string requestKey = std::string(StandardPropertyNames::CTRL_MAP_LIST) + ":" + ctrlMapId;
        if (isPropertyRequestPending(muid, requestKey)) {
            std::cout << "[PROPERTY REQUEST] CtrlMapList[" << ctrlMapId << "] already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        addPendingPropertyRequest(muid, requestKey);
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] GetPropertyData(CtrlMapList:'" << ctrlMapId << "') to MUID: 0x" << std::hex << muid << std::dec << std::endl;
        auto request_id = property_client.sendGetPropertyData(StandardPropertyNames::CTRL_MAP_LIST, ctrlMapId);
        registerPropertyRequestId(muid, request_id, requestKey);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] requestCtrlMapList failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

void MidiCIManager::requestAllCtrlList(uint32_t muid) {
    try {
        auto connection = device_->getConnection(muid);
        if (!connection) {
            std::cout << "[PROPERTY REQUEST] No connection for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        // Avoid duplicate in-flight requests
        if (isPropertyRequestPending(muid, StandardPropertyNames::ALL_CTRL_LIST)) {
            std::cout << "[PROPERTY REQUEST] AllCtrlList already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        addPendingPropertyRequest(muid, StandardPropertyNames::ALL_CTRL_LIST);
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] GetPropertyData(AllCtrlList) to MUID: 0x" << std::hex << muid << std::dec << std::endl;
        auto request_id = property_client.sendGetPropertyData(StandardPropertyNames::ALL_CTRL_LIST, "");
        registerPropertyRequestId(muid, request_id, StandardPropertyNames::ALL_CTRL_LIST);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] requestAllCtrlList failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

void MidiCIManager::requestProgramList(uint32_t muid) {
    try {
        auto connection = device_->getConnection(muid);
        if (!connection) {
            std::cout << "[PROPERTY REQUEST] No connection for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        // Avoid duplicate in-flight requests
        if (isPropertyRequestPending(muid, StandardPropertyNames::PROGRAM_LIST)) {
            std::cout << "[PROPERTY REQUEST] ProgramList already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        addPendingPropertyRequest(muid, StandardPropertyNames::PROGRAM_LIST);
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] GetPropertyData(ProgramList) to MUID: 0x" << std::hex << muid << std::dec << std::endl;
        auto request_id = property_client.sendGetPropertyData(StandardPropertyNames::PROGRAM_LIST, "");
        registerPropertyRequestId(muid, request_id, StandardPropertyNames::PROGRAM_LIST);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] requestProgramList failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

void MidiCIManager::requestSaveState(uint32_t muid) {
    try {
        auto connection = device_->getConnection(muid);
        if (!connection) {
            std::cout << "[PROPERTY REQUEST] No connection for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        if (isPropertyRequestPending(muid, StandardPropertyNames::STATE)) {
            std::cout << "[PROPERTY REQUEST] State already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        addPendingPropertyRequest(muid, StandardPropertyNames::STATE);
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] GetPropertyData(State, fullState) to MUID: 0x" << std::hex << muid << std::dec << std::endl;

        property_client.getPropertyData(
            StandardPropertyNames::STATE,
            midicci::commonproperties::MidiCIStatePredefinedNames::FULL_STATE,
            [this, &property_client, muid](const GetPropertyDataReply& reply) {
                this->removePendingPropertyRequest(muid, StandardPropertyNames::STATE);
                this->log("[State Response] Received State property data reply");

                if (this->state_save_callback_) {
                    auto rules = property_client.getPropertyRules();
                    std::vector<uint8_t> stateData = rules->decodeBody(reply.getHeader(), reply.getBody());
                    this->log("[State Response] State data size: " + std::to_string(stateData.size()) + " bytes");
                    this->state_save_callback_(muid, stateData);
                }
            },
            "Mcoded7"
        );
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] requestSaveState failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

std::optional<std::vector<midicci::commonproperties::MidiCIStateEntry>> MidiCIManager::getStateList(uint32_t muid) {
    instrumentation_log_property_call(muid, "StateList(read)", "getStateList");

    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }

    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }

    try {
        auto* properties = connection->getPropertyClientFacade().getProperties();
        if (!properties) {
            std::cout << "[PROPERTY ACCESS] No ObservablePropertyList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }

        return midicci::commonproperties::StandardPropertiesExtensions::getStateList(*properties);
    } catch (const std::exception& e) {
        std::cerr << "[PROPERTY ACCESS ERROR] getStateList failed: " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::vector<uint8_t>> MidiCIManager::getState(uint32_t muid, const std::string& stateId) {
    instrumentation_log_property_call(muid, "State(" + stateId + ")(read)", "getState");

    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }

    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }

    try {
        auto* properties = connection->getPropertyClientFacade().getProperties();
        if (!properties) {
            std::cout << "[PROPERTY ACCESS] No ObservablePropertyList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }

        return midicci::commonproperties::StandardPropertiesExtensions::getState(*properties, stateId);
    } catch (const std::exception& e) {
        std::cerr << "[PROPERTY ACCESS ERROR] getState failed: " << e.what() << std::endl;
        return std::nullopt;
    }
}

void MidiCIManager::requestStateList(uint32_t muid) {
    try {
        auto connection = device_->getConnection(muid);
        if (!connection) {
            std::cout << "[PROPERTY REQUEST] No connection for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        // Avoid duplicate in-flight requests
        if (isPropertyRequestPending(muid, StandardPropertyNames::STATE_LIST)) {
            std::cout << "[PROPERTY REQUEST] StateList already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        addPendingPropertyRequest(muid, StandardPropertyNames::STATE_LIST);
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] GetPropertyData(StateList) to MUID: 0x" << std::hex << muid << std::dec << std::endl;
        auto request_id = property_client.sendGetPropertyData(StandardPropertyNames::STATE_LIST, "");
        registerPropertyRequestId(muid, request_id, StandardPropertyNames::STATE_LIST);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] requestStateList failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

void MidiCIManager::sendState(uint32_t muid, const std::string& stateId, const std::vector<uint8_t>& data) {
    try {
        auto connection = device_->getConnection(muid);
        if (!connection) {
            std::cout << "[PROPERTY SEND] No connection for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        auto& property_client = connection->getPropertyClientFacade();
        std::cout << "[MIDI-CI SENT] SetPropertyData(State, " << stateId << ") to MUID: 0x" << std::hex << muid << std::dec
                  << " (" << data.size() << " bytes)" << std::endl;
        property_client.sendSetPropertyData(StandardPropertyNames::STATE, stateId, data, PropertyDataEncoding::MCODED7);
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] sendState failed for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

void MidiCIManager::setupPropertyCallbacks(uint32_t muid) {
    if (!initialized_ || !device_) {
        std::cerr << "[PROPERTY CALLBACKS] Cannot setup callbacks - not initialized" << std::endl;
        return;
    }
    
    // Get connection to the remote device
    auto connection = device_->getConnection(muid);
    if (!connection) {
        std::cout << "[PROPERTY CALLBACKS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return;
    }
    
    try {
        // Get the property client facade
        auto& property_client = connection->getPropertyClientFacade();
        
        // Get the observable property list  
        auto* properties = property_client.getProperties();
        if (!properties) {
            std::cout << "[PROPERTY CALLBACKS] No observable property list available for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        
        // Register callback for property value updates
        properties->addPropertyUpdatedCallback([this, muid](const std::string& propertyId, const std::string& resId) {
            std::cout << "[PROPERTY VALUE UPDATED] Property '" << propertyId << "' (resId '" << resId << "') updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;

            // Clear any pending requests for this specific property
            // Use the same key format as when the request was added
            std::string requestKey = resId.empty() ? propertyId : (propertyId + ":" + resId);
            this->removePendingPropertyRequest(muid, requestKey);
            // Mark property as fetched at least once
            this->mark_property_fetched(muid, requestKey);

            // If DeviceInfo was updated, refresh the device list display
            if (propertyId == "DeviceInfo") {
                std::cout << "[DEVICE INFO UPDATED] Updating device display name for MUID: 0x" << std::hex << muid << std::dec << std::endl;

                // Update the device info in discovered_devices_
                auto connection = this->device_->getConnection(muid);
                if (connection) {
                    const auto* device_info = connection->getDeviceInfo();
                    if (device_info) {
                        for (auto& device : this->discovered_devices_) {
                            if (device.muid == muid) {
                                device.manufacturer = device_info->manufacturer;
                                device.model = device_info->model;
                                device.version = device_info->version;
                                std::cout << "[DEVICE INFO UPDATED] Device name updated to: " << device.getDisplayName() << std::endl;
                                break;
                            }
                        }

                        // Trigger devices_changed_callback to refresh the UI dropdown
                        if (this->devices_changed_callback_) {
                            std::cout << "[DEVICE INFO UPDATED] Calling devices_changed_callback to refresh UI" << std::endl;
                            this->devices_changed_callback_();
                        }
                    }
                }
            }

            // Notify UI about this specific property change
            if (this->properties_changed_callback_) {
                this->properties_changed_callback_(muid, propertyId, resId);
            }
        });
        
        // Register callback for property catalog updates (when metadata changes)
        properties->addPropertyCatalogUpdatedCallback([this, muid]() {
            std::cout << "[PROPERTY CATALOG UPDATED] Property catalog updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            
            // Notify UI about property changes
            if (this->properties_changed_callback_) {
                this->properties_changed_callback_(muid, "", "");
            }
        });
        
        std::cout << "[PROPERTY CALLBACKS] Successfully set up property callbacks for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[PROPERTY CALLBACKS ERROR] Failed to setup callbacks for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

bool MidiCIManager::isPropertyRequestPending(uint32_t muid, const std::string& property_name) {
    cleanupExpiredPropertyRequests();
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    return std::any_of(pending_property_requests_.begin(), pending_property_requests_.end(),
                       [muid, &property_name](const PendingPropertyRequest& req) {
                           return req.muid == muid && req.property_name == property_name;
                       });
}

void MidiCIManager::addPendingPropertyRequest(uint32_t muid, const std::string& property_name) {
    cleanupExpiredPropertyRequests();
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    // First check if it already exists to avoid duplicates
    auto exists = std::any_of(pending_property_requests_.begin(), pending_property_requests_.end(),
                              [muid, &property_name](const PendingPropertyRequest& req) {
                                  return req.muid == muid && req.property_name == property_name;
                              });
    if (!exists) {
        pending_property_requests_.emplace_back(muid, property_name);
        std::cout << "[PROPERTY REQUEST] Added pending request for MUID: 0x" << std::hex << muid << std::dec 
                  << ", property: " << property_name << std::endl;
    }
}

void MidiCIManager::removePendingPropertyRequest(uint32_t muid, const std::string& property_name) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    auto it = std::remove_if(pending_property_requests_.begin(), pending_property_requests_.end(),
                            [muid, &property_name](const PendingPropertyRequest& req) {
                                return req.muid == muid && req.property_name == property_name;
                            });
    if (it != pending_property_requests_.end()) {
        pending_property_requests_.erase(it, pending_property_requests_.end());
        std::cout << "[PROPERTY REQUEST] Removed pending request for MUID: 0x" << std::hex << muid << std::dec 
                  << ", property: " << property_name << std::endl;
    }
    clearRequestIdTracking(muid, property_name);
}

void MidiCIManager::cleanupExpiredPropertyRequests() {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    const auto now = std::chrono::steady_clock::now();
    
    auto it = std::remove_if(pending_property_requests_.begin(), pending_property_requests_.end(),
                            [now](const PendingPropertyRequest& req) {
                                return (now - req.request_time) > PROPERTY_REQUEST_TIMEOUT;
                            });
    
    if (it != pending_property_requests_.end()) {
        size_t removed_count = pending_property_requests_.end() - it;
        pending_property_requests_.erase(it, pending_property_requests_.end());
        std::cout << "[PROPERTY REQUEST] Cleaned up " << removed_count << " expired property requests" << std::endl;
    }
}

bool MidiCIManager::has_property_been_fetched(uint32_t muid, const std::string& property_name) const {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    return fetched_properties_.find({muid, property_name}) != fetched_properties_.end();
}

void MidiCIManager::mark_property_fetched(uint32_t muid, const std::string& property_name) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    fetched_properties_.insert({muid, property_name});
}

void MidiCIManager::registerPropertyRequestId(uint32_t muid, uint8_t request_id, const std::string& request_key) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    inflight_request_map_[muid][request_id] = request_key;
}

std::optional<std::string> MidiCIManager::getRequestKeyForId(uint32_t muid, uint8_t request_id) const {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    auto device_it = inflight_request_map_.find(muid);
    if (device_it == inflight_request_map_.end()) {
        return std::nullopt;
    }
    auto req_it = device_it->second.find(request_id);
    if (req_it == device_it->second.end()) {
        return std::nullopt;
    }
    return req_it->second;
}

void MidiCIManager::clearRequestIdTracking(uint32_t muid, const std::string& request_key) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    auto device_it = inflight_request_map_.find(muid);
    if (device_it == inflight_request_map_.end()) {
        return;
    }
    for (auto it = device_it->second.begin(); it != device_it->second.end();) {
        if (it->second == request_key) {
            it = device_it->second.erase(it);
        } else {
            ++it;
        }
    }
    if (device_it->second.empty()) {
        inflight_request_map_.erase(device_it);
    }
}

void MidiCIManager::extendPendingPropertyRequest(uint32_t muid, const std::string& property_name) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    auto it = std::find_if(pending_property_requests_.begin(), pending_property_requests_.end(),
                           [muid, &property_name](const PendingPropertyRequest& req) {
                               return req.muid == muid && req.property_name == property_name;
                           });
    if (it != pending_property_requests_.end()) {
        it->request_time = std::chrono::steady_clock::now();
    }
}

void MidiCIManager::clearDiscoveredDevices() {
    std::cout << "[MIDI-CI] Clearing all discovered devices and pending property requests" << std::endl;
    discovered_devices_.clear();
    pending_property_requests_.clear();
    fetched_properties_.clear();
    
    // Notify UI about device list change
    if (devices_changed_callback_) {
        devices_changed_callback_();
    }
}

void MidiCIManager::instrumentation_log_property_call(uint32_t muid, const std::string& property_name, const std::string& caller) const {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    instrumentation_call_counter_++;
    auto key = std::make_pair(muid, property_name);
    instrumentation_property_call_counts_[key]++;
    auto now = std::chrono::steady_clock::now();
    
    auto prev_time_it = instrumentation_last_call_time_.find(key);
    std::string time_since = "FIRST";
    if (prev_time_it != instrumentation_last_call_time_.end()) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - prev_time_it->second);
        time_since = std::to_string(duration.count()) + "ms";
    }
    
    instrumentation_last_call_time_[key] = now;
    
    std::cout << "[INSTRUMENTATION #" << instrumentation_call_counter_ << "] " 
              << caller << " -> " << property_name 
              << " (MUID: 0x" << std::hex << muid << std::dec << ") "
              << "Count: " << instrumentation_property_call_counts_[key] 
              << ", Time since last: " << time_since << std::endl;
}

void MidiCIManager::instrumentation_print_statistics() const {
    std::cout << "\n[INSTRUMENTATION STATS] Total property calls: " << instrumentation_call_counter_ << std::endl;
    std::cout << "[INSTRUMENTATION STATS] Call counts by property:" << std::endl;

    for (const auto& entry : instrumentation_property_call_counts_) {
        uint32_t muid = entry.first.first;
        const std::string& property = entry.first.second;
        int count = entry.second;

        std::cout << "  MUID 0x" << std::hex << muid << std::dec
                  << " -> " << property << ": " << count << " calls" << std::endl;
    }
    std::cout << std::endl;
}
