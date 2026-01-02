#include "midicci/tooling/CIDeviceManager.hpp"
#include "midicci/tooling/CIToolRepository.hpp"
#include "midicci/tooling/MidiDeviceManager.hpp"
#include "midicci/tooling/CIDeviceModel.hpp"
#include <midicci/midicci.hpp>
#include <mutex>
#include <iostream>
#include "MidiDeviceManager.hpp"

namespace midicci::tooling {

CIDeviceManager::CIDeviceManager(CIToolRepository& repository,
                                 MidiCIDeviceConfiguration& config,
                                 std::shared_ptr<MidiDeviceManager> midi_manager)
    : repository_(repository), config_(config), midi_device_manager_(midi_manager) {}

CIDeviceManager::~CIDeviceManager() = default;

void CIDeviceManager::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto ci_output_sender = [this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        // Convert uint8_t SysEx data to UMP format
        std::vector<uint8_t> sysex_data;
        sysex_data.push_back(0xF0);
        sysex_data.insert(sysex_data.end(), data.begin(), data.end());
        sysex_data.push_back(0xF7);
        
        // Convert to UMP SysEx7 format (skip F0 and F7)
        std::vector<uint8_t> sysex_payload(sysex_data.begin() + 1, sysex_data.end() - 1);
        auto umps = midicci::ump::UmpFactory::sysex7(group, sysex_payload);
        std::vector<uint32_t> ump_data;
        for (const auto& ump : umps) {
            ump_data.push_back(ump.int1);
            ump_data.push_back(ump.int2);
            ump_data.push_back(ump.int3);
            ump_data.push_back(ump.int4);
        }
        
        std::string hex_str;
        for (uint8_t byte : data) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", byte);
            hex_str += hex;
        }
        repository_.log("[sent CI SysEx (grp:" + std::to_string(group) + ")] " + hex_str, MessageDirection::Out);
        
        return midi_device_manager_->send_sysex(group, ump_data);
    };
    
    auto midi_message_report_sender = [this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        // Convert uint8_t data to UMP format
        std::vector<uint8_t> data_vector(data.begin(), data.end());
        auto umps = midicci::ump::UmpFactory::sysex7(group, data_vector);
        std::vector<uint32_t> ump_data;
        for (const auto& ump : umps) {
            ump_data.push_back(ump.int1);
            ump_data.push_back(ump.int2);
            ump_data.push_back(ump.int3);
            ump_data.push_back(ump.int4);
        }
        
        std::string hex_str;
        for (uint8_t byte : data) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", byte);
            hex_str += hex;
        }
        repository_.log("[sent MIDI Message Report (grp:" + std::to_string(group) + ")] " + hex_str, MessageDirection::Out);
        
        return midi_device_manager_->send_sysex(group, ump_data);
    };
    
    auto logger_wrapper = [this](const LogData& log_data) {
        MessageDirection direction = log_data.is_outgoing ? MessageDirection::Out : MessageDirection::In;
        if (log_data.has_message()) {
            // For structured messages, extract MUIDs and use the message's log string
            const auto& message = log_data.get_message();
            repository_.log(message.get_log_message(), direction, 
                          message.get_source_muid(), message.get_destination_muid());
        } else {
            // For plain string messages, no MUID information
            repository_.log(log_data.get_string(), direction);
        }
    };
    
    device_model_ = std::make_shared<CIDeviceModel>(
        *this, config_, repository_.get_muid(),
        ci_output_sender, midi_message_report_sender, logger_wrapper);
    
    device_model_->initialize();
    
    midi_device_manager_->set_sysex_callback(
        [this](uint8_t group, const std::vector<uint32_t>& ump_data) {
            // Process UMP packets directly (4 words = 1 UMP packet)
            for (size_t i = 0; i + 3 < ump_data.size(); i += 4) {
                midicci::ump::Ump ump(ump_data[i], ump_data[i+1], ump_data[i+2], ump_data[i+3]);
                process_single_ump_packet(ump);
            }
        });
    
    midi_device_manager_->add_input_opened_callback([this]() {
        setup_input_event_listener();
    });
    
    std::cout << "CIDeviceManager initialized" << std::endl;
}

void CIDeviceManager::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (device_model_) {
        device_model_->shutdown();
        device_model_.reset();
    }
    std::cout << "CIDeviceManager shutdown" << std::endl;
}

std::shared_ptr<CIDeviceModel> CIDeviceManager::get_device_model() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    return device_model_;
}

void CIDeviceManager::process_midi1_input(const std::vector<uint8_t>& data, size_t start, size_t length) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string hex_str;
    for (size_t i = start; i < start + length && i < data.size(); ++i) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X ", data[i]);
        hex_str += hex;
    }
    repository_.log("[received MIDI1] " + hex_str, MessageDirection::In);
    
    if (data.size() > start + 3 &&
        data[start] == 0xF0 &&
        data[start + 1] == 0x7E &&
        data[start + 3] == 0x0D) {
        
        size_t end_pos = start;
        for (size_t i = start; i < start + length && i < data.size(); ++i) {
            if (data[i] == 0xF7) {
                end_pos = i;
                break;
            }
        }
        
        if (end_pos > start) {
            std::vector<uint8_t> ci_data(data.begin() + start + 1, data.begin() + end_pos);
            std::string ci_hex_str;
            for (uint8_t byte : ci_data) {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X ", byte);
                ci_hex_str += hex;
            }
            repository_.log("[received CI SysEx] " + ci_hex_str, MessageDirection::In);
            
            if (device_model_) {
                device_model_->process_ci_message(0, ci_data);
            }
        }
    }
}

void CIDeviceManager::process_single_ump_packet(const midicci::ump::Ump& ump) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Log the UMP packet
    bool is_ci_sysex = (ump.get_message_type() == midicci::ump::MessageType::SYSEX7 ||
                        ump.get_message_type() == midicci::ump::MessageType::SYSEX8_MDS);
    
    switch (ump.get_message_type()) {
        case midicci::ump::MessageType::SYSEX7: {
            if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::START)) {
                buffered_sysex7_.clear();
            }
            
            std::vector<midicci::ump::Ump> single_ump = {ump};
            auto sysex_data = midicci::ump::UmpRetriever::get_sysex7_data(single_ump);
            buffered_sysex7_.insert(buffered_sysex7_.end(), 
                                           sysex_data.begin(), sysex_data.end());
            
            if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::END) ||
                ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::COMPLETE_PACKET)) {
                
                if (buffered_sysex7_.size() > 2 &&
                    buffered_sysex7_[0] == 0x7E &&
                    buffered_sysex7_[2] == 0x0D) {
                    
                    std::string sysex7_hex;
                    for (uint8_t byte : buffered_sysex7_) {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%02X ", byte);
                        sysex7_hex += hex;
                    }
                    repository_.log("[received CI SysEx7] " + sysex7_hex, MessageDirection::In);
                    
                    if (device_model_) {
                        device_model_->process_ci_message(ump.get_group(), buffered_sysex7_);
                    }
                    buffered_sysex7_.clear();
                }
                else
                    repository_.log(
                        std::format("[received non-CI SysEx7] {} {} {}",
                            buffered_sysex7_.size(),
                            buffered_sysex7_.empty() ? ' ' : buffered_sysex7_[0],
                            buffered_sysex7_.size() < 2 ? ' ' : buffered_sysex7_[1]
                            ),
                        MessageDirection::In);
            }
            break;
        }
        
        case midicci::ump::MessageType::SYSEX8_MDS: {
            if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::START)) {
                buffered_sysex8_.clear();
            }
            
            std::vector<midicci::ump::Ump> single_ump = {ump};
            auto sysex_data = midicci::ump::UmpRetriever::get_sysex8_data(single_ump);
            buffered_sysex8_.insert(buffered_sysex8_.end(), 
                                           sysex_data.begin(), sysex_data.end());
            
            if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::END) ||
                ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::COMPLETE_PACKET)) {
                
                if (buffered_sysex8_.size() > 2 &&
                    buffered_sysex8_[0] == 0x7E &&
                    buffered_sysex8_[2] == 0x0D) {
                    
                    std::string sysex8_hex;
                    for (uint8_t byte : buffered_sysex8_) {
                        char hex[4];
                        snprintf(hex, sizeof(hex), "%02X ", byte);
                        sysex8_hex += hex;
                    }
                    repository_.log("[received CI SysEx8] " + sysex8_hex, MessageDirection::In);
                    
                    if (device_model_) {
                        device_model_->process_ci_message(ump.get_group(), buffered_sysex8_);
                    }
                    buffered_sysex8_.clear();
                }
            }
            break;
        }
        
        default:
            // Log other message types but don't process them as CI messages
            repository_.log("[received UMP message type: " + std::to_string(static_cast<int>(ump.get_message_type())) + "]", MessageDirection::In);
            break;
    }
}

void CIDeviceManager::process_ump_input(const std::vector<uint8_t>& data, size_t start, size_t length) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // avoid logging every UMP byte dump; detailed logs handled per-message
    
    auto umps = midicci::ump::parse_umps_from_bytes(data.data(), start, length);
    
    for (const auto& ump : umps) {
        switch (ump.get_message_type()) {
            case midicci::ump::MessageType::SYSEX7: {
                if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::START)) {
                    buffered_sysex7_.clear();
                }
                
                std::vector<midicci::ump::Ump> single_ump = {ump};
                auto sysex_data = midicci::ump::UmpRetriever::get_sysex7_data(single_ump);
                buffered_sysex7_.insert(buffered_sysex7_.end(), 
                                               sysex_data.begin(), sysex_data.end());
                
                if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::END) ||
                    ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::COMPLETE_PACKET)) {
                    
                    if (buffered_sysex7_.size() > 2 &&
                        buffered_sysex7_[0] == 0x7E &&
                        buffered_sysex7_[2] == 0x0D) {
                        
                        std::string sysex7_hex;
                        for (uint8_t byte : buffered_sysex7_) {
                            char hex[4];
                            snprintf(hex, sizeof(hex), "%02X ", byte);
                            sysex7_hex += hex;
                        }
                        repository_.log("[received CI SysEx7] " + sysex7_hex, MessageDirection::In);
                        
                        if (device_model_) {
                            device_model_->process_ci_message(ump.get_group(), buffered_sysex7_);
                        }
                        buffered_sysex7_.clear();
                    }
                }
                break;
            }
            
            case midicci::ump::MessageType::SYSEX8_MDS: {
                if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::START)) {
                    buffered_sysex8_.clear();
                }
                
                std::vector<midicci::ump::Ump> single_ump = {ump};
                auto sysex_data = midicci::ump::UmpRetriever::get_sysex8_data(single_ump);
                buffered_sysex8_.insert(buffered_sysex8_.end(), 
                                               sysex_data.begin(), sysex_data.end());
                
                if (ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::END) ||
                    ump.get_status_code() == static_cast<uint8_t>(midicci::ump::BinaryChunkStatus::COMPLETE_PACKET)) {
                    
                    if (buffered_sysex8_.size() > 2 &&
                        buffered_sysex8_[0] == 0x7E &&
                        buffered_sysex8_[2] == 0x0D) {
                        
                        std::string sysex8_hex;
                        for (uint8_t byte : buffered_sysex8_) {
                            char hex[4];
                            snprintf(hex, sizeof(hex), "%02X ", byte);
                            sysex8_hex += hex;
                        }
                        repository_.log("[received CI SysEx8] " + sysex8_hex, MessageDirection::In);
                        
                        if (device_model_) {
                            device_model_->process_ci_message(ump.get_group(), buffered_sysex8_);
                        }
                        buffered_sysex8_.clear();
                    }
                }
                break;
            }
            
            default:
                break;
        }
    }
}

void CIDeviceManager::setup_input_event_listener() {
    std::cout << "Input event listener set up for MIDI 1.0 and UMP protocols" << std::endl;
}

void CIDeviceManager::log_midi_message_report_chunk(const std::vector<uint8_t>& data) {
    std::string hex_str;
    for (uint8_t byte : data) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", byte);
        hex_str += hex;
        hex_str += " ";
    }
    repository_.log("MIDI Message Report: " + hex_str, MessageDirection::In);
}

} // namespace ci_tool
