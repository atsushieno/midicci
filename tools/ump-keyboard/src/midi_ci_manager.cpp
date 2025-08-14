#include "midi_ci_manager.h"
#include <iostream>
#include <iomanip>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace midicci::commonproperties;

MidiCIManager::MidiCIManager() 
    : muid_(0), initialized_(false) {
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
            std::uniform_int_distribution<uint32_t> dis(1, 0xFFFFFF); // Avoid 0 and reserved values
            muid_ = dis(gen) & 0x7F7F7F7F;
        } else {
            muid_ = muid;
        }
        
        // Create device configuration
        setupDeviceConfiguration();
        
        // Create MIDI-CI device
        device_ = std::make_unique<midicci::MidiCIDevice>(
            muid_, 
            *config_,
            [this](const std::string& message, bool is_outgoing) {
                // Use our log method with proper parameter semantics
                log(message, is_outgoing);
            }
        );
        
        // Setup callbacks
        setupCallbacks();
        
        // Set up SysEx sender if already provided
        if (sysex_sender_) {
            device_->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
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
        device_->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
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

void MidiCIManager::setPropertiesChangedCallback(std::function<void(uint32_t)> callback) {
    properties_changed_callback_ = callback;
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
    
    // Set up message callback for outgoing messages
    device_->set_message_callback([this](const midicci::Message& message) {
        std::string msgTypeName;
        switch(message.get_type()) {
            case midicci::MessageType::DiscoveryInquiry: msgTypeName = "DiscoveryInquiry"; break;
            case midicci::MessageType::DiscoveryReply: msgTypeName = "DiscoveryReply"; break;
            case midicci::MessageType::GetPropertyData: msgTypeName = "GetPropertyData"; break;
            case midicci::MessageType::GetPropertyDataReply: msgTypeName = "GetPropertyDataReply"; break;
            default: msgTypeName = "Unknown(" + std::to_string(static_cast<int>(message.get_type())) + ")"; break;
        }
        log("MIDI-CI Message sent: " + msgTypeName, true); // outgoing
        std::cout << "[MIDI-CI SENT] " << msgTypeName << " to MUID: 0x" << std::hex << message.get_destination_muid() << std::dec << std::endl;
    });
    
    // Set up message received callback
    device_->set_message_received_callback([this](const midicci::Message& message) {
        std::cout << "[UMP-KEYBOARD RECV] Message type: " << static_cast<int>(message.get_type()) << std::endl;
        log("MIDI-CI Message received: " + std::to_string(static_cast<int>(message.get_type())), false); // incoming
        
        // Handle Endpoint Reply messages to populate device combobox
        if (message.get_type() == midicci::MessageType::EndpointReply) {
            std::cout << "[ENDPOINT REPLY] Processing endpoint reply message" << std::endl;
            
            try {
                const auto* endpoint_reply = dynamic_cast<const midicci::EndpointReply*>(&message);
                if (endpoint_reply) {
                    uint32_t source_muid = endpoint_reply->get_source_muid();
                    
                    std::cout << "[ENDPOINT REPLY] Source MUID: 0x" << std::hex << source_muid << std::dec << " (" << source_muid << ")" << std::endl;
                    
                    // Find the device and mark it as endpoint ready
                    bool found = false;
                    for (auto& device : discovered_devices_) {
                        if (device.muid == source_muid) {
                            std::cout << "[ENDPOINT REPLY] Marking device MUID 0x" << std::hex << source_muid << std::dec << " as endpoint ready" << std::endl;
                            device.endpoint_ready = true;
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
        if (message.get_type() == midicci::MessageType::DiscoveryReply) {
            std::cout << "[DISCOVERY REPLY] Processing discovery reply message" << std::endl;
            
            // Try to cast to DiscoveryReply to get device details
            try {
                const auto* discovery_reply = dynamic_cast<const midicci::DiscoveryReply*>(&message);
                if (discovery_reply) {
                    uint32_t source_muid = discovery_reply->get_source_muid();
                    const auto& device_details = discovery_reply->get_device_details();
                    
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
    device_->set_connections_changed_callback([this]() {
        log("MIDI-CI Connections changed", false); // incoming change notification
        
        // Set up property callbacks for all connected devices
        auto& connections = device_->get_connections();
        for (const auto& conn_pair : connections) {
            uint32_t muid = conn_pair.first;
            std::cout << "[CONNECTIONS CHANGED] Setting up property callbacks for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            setupPropertyCallbacks(muid);
        }
        
        if (devices_changed_callback_) {
            devices_changed_callback_();
        }
    });
}

void MidiCIManager::log(const std::string& message, bool is_outgoing) {
    std::string prefix = is_outgoing ? "[MIDI-CI OUT] " : "[MIDI-CI IN] ";
    std::string full_message = prefix + message;
    
    if (log_callback_) {
        log_callback_(full_message);
    } else {
        std::cout << full_message << std::endl;
    }
}


// Property management methods using PropertyClientFacade for remote device access
std::optional<std::vector<midicci::commonproperties::MidiCIControl>> MidiCIManager::getAllCtrlList(uint32_t muid) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }
    
    // Get connection to the remote device
    auto connection = device_->get_connection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }
    
    try {
        // Clean up expired requests first
        cleanupExpiredPropertyRequests();
        
        auto* properties = connection->get_property_client_facade().get_properties();
        std::cout << "[PROPERTY ACCESS DEBUG] Checking if AllCtrlList exists in local cache" << std::endl;
        auto ret = StandardPropertiesExtensions::getAllCtrlList(*properties);
        std::cout << "[PROPERTY ACCESS DEBUG] StandardPropertiesExtensions::getAllCtrlList returned: " << (ret.has_value() ? "valid data" : "nullopt") << std::endl;
        if (ret.has_value()) {
            std::cout << "[PROPERTY ACCESS DEBUG] AllCtrlList data size: " << ret->size() << " controls" << std::endl;
        }
        
        // If we have no data OR we have valid but empty data, we need to request it
        if (!ret || (ret.has_value() && ret->empty())) {
            // Check if we already have a pending request for this property
            if (isPropertyRequestPending(muid, StandardPropertyNames::ALL_CTRL_LIST)) {
                std::cout << "[PROPERTY ACCESS] AllCtrlList request already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
                return std::nullopt;
            }
            
            std::cout << "[PROPERTY ACCESS DEBUG] No pending request found, proceeding to send AllCtrlList request" << std::endl;
            
            // Add to pending requests to prevent duplicates
            addPendingPropertyRequest(muid, StandardPropertyNames::ALL_CTRL_LIST);
            
            auto& property_client = connection->get_property_client_facade();
            
            std::cout << "[PROPERTY ACCESS DEBUG] About to request AllCtrlList for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            std::cout << "[PROPERTY ACCESS DEBUG] Property name: '" << StandardPropertyNames::ALL_CTRL_LIST << "'" << std::endl;
            std::string prop_name = StandardPropertyNames::ALL_CTRL_LIST;
            std::cout << "[PROPERTY ACCESS DEBUG] Property name length: " << prop_name.length() << std::endl;
            std::cout << "[PROPERTY ACCESS DEBUG] Property name bytes: ";
            for (char c : prop_name) {
                std::cout << std::hex << (int)(unsigned char)c << " ";
            }
            std::cout << std::dec << std::endl;
            
            // Additional safety check - ensure the property name is valid
            if (prop_name.empty()) {
                std::cerr << "[PROPERTY ACCESS ERROR] ALL_CTRL_LIST property name is empty!" << std::endl;
                return std::nullopt;
            }
            
            property_client.send_get_property_data(StandardPropertyNames::ALL_CTRL_LIST, "");
            std::cout << "[PROPERTY ACCESS] Requested AllCtrlList from remote device (async response expected)" << std::endl;
        } else {
            // We have valid non-empty data - remove from pending requests 
            removePendingPropertyRequest(muid, StandardPropertyNames::ALL_CTRL_LIST);
            std::cout << "[PROPERTY ACCESS] Found cached AllCtrlList with " << ret->size() << " controls" << std::endl;
        }
        return ret;
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to get AllCtrlList for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

std::optional<std::vector<midicci::commonproperties::MidiCIProgram>> MidiCIManager::getProgramList(uint32_t muid) {
    std::lock_guard<std::recursive_mutex> lock(midi_ci_mutex_);
    
    if (!initialized_ || !device_) {
        std::cerr << "[MIDI-CI ERROR] Cannot get properties - not initialized" << std::endl;
        return std::nullopt;
    }
    
    // Get connection to the remote device
    auto connection = device_->get_connection(muid);
    if (!connection) {
        std::cout << "[PROPERTY ACCESS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return std::nullopt;
    }
    
    try {
        // Clean up expired requests first
        cleanupExpiredPropertyRequests();
        
        // Check if we already have a pending request for this property
        if (isPropertyRequestPending(muid, StandardPropertyNames::PROGRAM_LIST)) {
            std::cout << "[PROPERTY ACCESS] ProgramList request already pending for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return std::nullopt;
        }
        
        // Access the property client facade for this connection
        auto& property_client = connection->get_property_client_facade();
        auto* properties = property_client.get_properties();
        
        // Look for ProgramList in the client's property values
        auto values = properties->getValues();
        auto it = std::find_if(values.begin(), values.end(),
                               [](const midicci::PropertyValue& pv) { 
                                   return pv.id == StandardPropertyNames::PROGRAM_LIST; 
                               });
        
        if (it != values.end()) {
            // Parse the property data
            auto program_list = StandardProperties::parseProgramList(it->body);
            std::cout << "[PROPERTY ACCESS] Retrieved " << program_list.size() << " programs for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            
            // Remove from pending requests since we got a successful response
            removePendingPropertyRequest(muid, StandardPropertyNames::PROGRAM_LIST);
            
            return program_list;
        } else {
            std::cout << "[PROPERTY ACCESS] ProgramList not found in client properties for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            
            // Add to pending requests to prevent duplicates
            addPendingPropertyRequest(muid, StandardPropertyNames::PROGRAM_LIST);
            
            // Try to request the property if not already available
            property_client.send_get_property_data(StandardPropertyNames::PROGRAM_LIST, "");
            std::cout << "[PROPERTY ACCESS] Requested ProgramList from remote device" << std::endl;
            return std::nullopt;
        }
    } catch (const std::exception& e) {
        std::cerr << "[MIDI-CI ERROR] Failed to get ProgramList for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
        return std::nullopt;
    }
}

void MidiCIManager::setupPropertyCallbacks(uint32_t muid) {
    if (!initialized_ || !device_) {
        std::cerr << "[PROPERTY CALLBACKS] Cannot setup callbacks - not initialized" << std::endl;
        return;
    }
    
    // Get connection to the remote device
    auto connection = device_->get_connection(muid);
    if (!connection) {
        std::cout << "[PROPERTY CALLBACKS] No connection found for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        return;
    }
    
    try {
        // Get the property client facade
        auto& property_client = connection->get_property_client_facade();
        
        // Get the observable property list  
        auto* properties = property_client.get_properties();
        if (!properties) {
            std::cout << "[PROPERTY CALLBACKS] No observable property list available for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            return;
        }
        
        // Register callback for property value updates
        properties->addPropertyUpdatedCallback([this, muid](const std::string& propertyId) {
            std::cout << "[PROPERTY VALUE UPDATED] Property '" << propertyId << "' updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            
            // Clear any pending requests for this specific property
            this->removePendingPropertyRequest(muid, propertyId);
            
            // Notify UI about property changes
            if (this->properties_changed_callback_) {
                this->properties_changed_callback_(muid);
            }
        });
        
        // Register callback for property catalog updates (when metadata changes)
        properties->addPropertyCatalogUpdatedCallback([this, muid]() {
            std::cout << "[PROPERTY CATALOG UPDATED] Property catalog updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            
            // Notify UI about property changes
            if (this->properties_changed_callback_) {
                this->properties_changed_callback_(muid);
            }
        });
        
        std::cout << "[PROPERTY CALLBACKS] Successfully set up property callbacks for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[PROPERTY CALLBACKS ERROR] Failed to setup callbacks for MUID 0x" << std::hex << muid << std::dec << ": " << e.what() << std::endl;
    }
}

bool MidiCIManager::isPropertyRequestPending(uint32_t muid, const std::string& property_name) {
    return std::any_of(pending_property_requests_.begin(), pending_property_requests_.end(),
                       [muid, &property_name](const PendingPropertyRequest& req) {
                           return req.muid == muid && req.property_name == property_name;
                       });
}

void MidiCIManager::addPendingPropertyRequest(uint32_t muid, const std::string& property_name) {
    // First check if it already exists to avoid duplicates
    if (!isPropertyRequestPending(muid, property_name)) {
        pending_property_requests_.emplace_back(muid, property_name);
        std::cout << "[PROPERTY REQUEST] Added pending request for MUID: 0x" << std::hex << muid << std::dec 
                  << ", property: " << property_name << std::endl;
    }
}

void MidiCIManager::removePendingPropertyRequest(uint32_t muid, const std::string& property_name) {
    auto it = std::remove_if(pending_property_requests_.begin(), pending_property_requests_.end(),
                            [muid, &property_name](const PendingPropertyRequest& req) {
                                return req.muid == muid && req.property_name == property_name;
                            });
    if (it != pending_property_requests_.end()) {
        pending_property_requests_.erase(it, pending_property_requests_.end());
        std::cout << "[PROPERTY REQUEST] Removed pending request for MUID: 0x" << std::hex << muid << std::dec 
                  << ", property: " << property_name << std::endl;
    }
}

void MidiCIManager::cleanupExpiredPropertyRequests() {
    const auto now = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::seconds(30); // 30 second timeout
    
    auto it = std::remove_if(pending_property_requests_.begin(), pending_property_requests_.end(),
                            [now, timeout](const PendingPropertyRequest& req) {
                                return (now - req.request_time) > timeout;
                            });
    
    if (it != pending_property_requests_.end()) {
        size_t removed_count = pending_property_requests_.end() - it;
        pending_property_requests_.erase(it, pending_property_requests_.end());
        std::cout << "[PROPERTY REQUEST] Cleaned up " << removed_count << " expired property requests" << std::endl;
    }
}

void MidiCIManager::clearDiscoveredDevices() {
    std::cout << "[MIDI-CI] Clearing all discovered devices and pending property requests" << std::endl;
    discovered_devices_.clear();
    pending_property_requests_.clear();
    
    // Notify UI about device list change
    if (devices_changed_callback_) {
        devices_changed_callback_();
    }
}

