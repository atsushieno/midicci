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
        std::vector<uint8_t> sysex_data;
        sysex_data.push_back(0xF0);
        sysex_data.insert(sysex_data.end(), data.begin(), data.end());
        sysex_data.push_back(0xF7);
        
        std::string hex_str;
        for (uint8_t byte : data) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", byte);
            hex_str += hex;
        }
        repository_.log("[sent CI SysEx (grp:" + std::to_string(group) + ")] " + hex_str, MessageDirection::Out);
        
        return midi_device_manager_->send_sysex(group, sysex_data);
    };
    
    auto midi_message_report_sender = [this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        std::string hex_str;
        for (uint8_t byte : data) {
            char hex[4];
            snprintf(hex, sizeof(hex), "%02X ", byte);
            hex_str += hex;
        }
        repository_.log("[sent MIDI Message Report (grp:" + std::to_string(group) + ")] " + hex_str, MessageDirection::Out);
        
        return midi_device_manager_->send_sysex(group, data);
    };
    
    auto logger_wrapper = [this](const std::string& message, bool is_outgoing) {
        MessageDirection direction = is_outgoing ? MessageDirection::Out : MessageDirection::In;
        repository_.log(message, direction);
    };
    
    device_model_ = std::make_shared<CIDeviceModel>(
        *this, config_, repository_.get_muid(),
        ci_output_sender, midi_message_report_sender, logger_wrapper);
    
    device_model_->initialize();
    
    midi_device_manager_->set_sysex_callback(
        [this](uint8_t group, const std::vector<uint8_t>& data) {
            process_midi1_input(data, 0, data.size());
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

void CIDeviceManager::process_ump_input(const std::vector<uint8_t>& data, size_t start, size_t length) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    std::string hex_str;
    for (size_t i = start; i < start + length && i < data.size(); ++i) {
        char hex[4];
        snprintf(hex, sizeof(hex), "%02X ", data[i]);
        hex_str += hex;
    }
    repository_.log("[received UMP] " + hex_str, MessageDirection::In);
    
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
