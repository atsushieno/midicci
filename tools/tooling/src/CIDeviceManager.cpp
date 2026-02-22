#include "midicci/tooling/CIDeviceManager.hpp"
#include "midicci/tooling/CIToolRepository.hpp"
#include "midicci/tooling/MidiDeviceManager.hpp"
#include "midicci/tooling/CIDeviceModel.hpp"
#include <midicci/midicci.hpp>
#include <mutex>
#include <iostream>
#include <format>
#include "MidiDeviceManager.hpp"

namespace midicci::tooling {

CIDeviceManager::CIDeviceManager(CIToolRepository& repository,
                                 MidiCIDeviceConfiguration& config,
                                 std::shared_ptr<MidiDeviceManager> midi_manager)
    : repository_(repository), config_(config), midi_device_manager_(midi_manager) {}

CIDeviceManager::~CIDeviceManager() = default;

void CIDeviceManager::initialize() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto logger_wrapper = [this](const LogData& log_data) {
        MessageDirection direction = log_data.is_outgoing ? MessageDirection::Out : MessageDirection::In;
        if (log_data.hasMessage()) {
            // For structured messages, extract MUIDs and use the message's log string
            const auto& message = log_data.getMessage();
            repository_.log(message.getLogMessage(), direction, 
                          message.getSourceMuid(), message.getDestinationMuid());
        } else {
            // For plain string messages, no MUID information
            repository_.log(log_data.getString(), direction);
        }
    };

    musicdevice::MidiCISessionSource source{
        [this](midicci::musicdevice::MidiInputCallback callback) {
            midi_device_manager_->add_ump_listener(std::move(callback));
        },
        [this](umppi::UmpWordSpan words, uint64_t timestamp_ns) {
            midi_device_manager_->send_ump(words, timestamp_ns);
        }
    };

    auto session = musicdevice::createMidiCiSession(
        source,
        repository_.getMuid(),
        config_,
        logger_wrapper);

    ci_session_ = std::shared_ptr<musicdevice::MidiCISession>(std::move(session));

    device_model_ = std::make_shared<CIDeviceModel>(
        *this,
        config_,
        repository_.getMuid(),
        ci_session_,
        logger_wrapper);
    
    device_model_->initialize();
    
    midi_device_manager_->set_sysex_callback(
        [this](uint8_t /*group*/, umppi::UmpWordSpan words) {
            auto umps = umppi::parseUmpsFromWords(words);
            for (const auto& ump : umps) {
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

void CIDeviceManager::process_single_ump_packet(const umppi::Ump& ump) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Log the UMP packet
    bool is_ci_sysex = (ump.getMessageType() == umppi::MessageType::SYSEX7 ||
                        ump.getMessageType() == umppi::MessageType::SYSEX8_MDS);
    
    switch (ump.getMessageType()) {
        case umppi::MessageType::SYSEX7: {
            if (ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::START)) {
                buffered_sysex7_.clear();
            }
            
            std::vector<umppi::Ump> single_ump = {ump};
            auto sysex_data = umppi::UmpRetriever::getSysex7Data(single_ump);
            buffered_sysex7_.insert(buffered_sysex7_.end(), 
                                           sysex_data.begin(), sysex_data.end());
            
            if (ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::END) ||
                ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::COMPLETE_PACKET)) {
                
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
                        device_model_->processCiMessage(ump.getGroup(), buffered_sysex7_);
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
        
        case umppi::MessageType::SYSEX8_MDS: {
            if (ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::START)) {
                buffered_sysex8_.clear();
            }
            
            std::vector<umppi::Ump> single_ump = {ump};
            auto sysex_data = umppi::UmpRetriever::getSysex8Data(single_ump);
            buffered_sysex8_.insert(buffered_sysex8_.end(), 
                                           sysex_data.begin(), sysex_data.end());
            
            if (ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::END) ||
                ump.getStatusCode() == static_cast<uint8_t>(umppi::BinaryChunkStatus::COMPLETE_PACKET)) {
                
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
                        device_model_->processCiMessage(ump.getGroup(), buffered_sysex8_);
                    }
                    buffered_sysex8_.clear();
                }
            }
            break;
        }
        
        default:
            // Log other message types but don't process them as CI messages
            repository_.log("[received UMP message type: " + std::to_string(static_cast<int>(ump.getMessageType())) + "]", MessageDirection::In);
            break;
    }
}

void CIDeviceManager::setup_input_event_listener() {
    std::cout << "Input event listener set up for MIDI 1.0 and UMP protocols" << std::endl;
}

void CIDeviceManager::logMidiMessageReportChunk(const std::vector<uint8_t>& data) {
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
