#include "midicci/musicdevice/MidiCISession.hpp"
#include "midicci/MidiCIConstants.hpp"
#include "midicci/ump/Ump.hpp"
#include "midicci/ump/UmpRetriever.hpp"
#include <random>
#include <iomanip>
#include <sstream>

namespace midicci::musicdevice {

std::unique_ptr<MidiCISession> create_midi_ci_session(
    const MidiCISessionSource& source,
    uint32_t muid,
    const MidiCIDeviceConfiguration& config
) {
    if (muid == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(1, 0x7F7F7F7F);
        muid = dis(gen);
    }
    
    auto device = std::make_unique<MidiCIDevice>(muid, const_cast<MidiCIDeviceConfiguration&>(config));
    
    // Set up MIDI-CI output sender
    device->set_sysex_sender([source](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        if (source.transport_protocol == MidiTransportProtocol::UMP) {
            // TODO: Implement UMP SysEx7 processing when UmpFactory is available
            // For now, send as is
            source.output_sender(data, 0, data.size(), 0);
        } else {
            // MIDI 1.0: add SysEx start byte
            std::vector<uint8_t> midi1_data;
            midi1_data.push_back(0xF0);
            midi1_data.insert(midi1_data.end(), data.begin(), data.end());
            source.output_sender(midi1_data, 0, midi1_data.size(), 0);
        }
        return true;
    });
    
    return std::make_unique<MidiCISession>(
        source.transport_protocol,
        source.input_listener_adder,
        std::move(device)
    );
}

MidiCISession::MidiCISession(
    MidiTransportProtocol input_protocol,
    MidiInputListenerAdder input_listener_adder,
    std::unique_ptr<MidiCIDevice> device
) : device_(std::move(device)),
    receiving_midi_message_reports_(false),
    last_chunked_message_channel_(0xFF)  // Invalid channel initially
{
    // Set up MIDI input processing
    input_listener_adder([this, input_protocol](const std::vector<uint8_t>& data, size_t start, size_t length, uint64_t timestamp) {
        if (input_protocol == MidiTransportProtocol::UMP) {
            process_ump_input(data, start, length);
        } else {
            process_midi1_input(data, start, length);
        }
    });
    
    // TODO: Set up message received handlers and MIDI message reporter
    // This requires access to device message handling which may need interface additions
}

void MidiCISession::process_ci_message(uint8_t group, const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    
    // Log the message
    auto logger = device_->get_logger();
    if (logger) {
        std::stringstream ss;
        ss << "[received CI SysEx (grp:" << static_cast<int>(group) << ")] ";
        for (uint8_t byte : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        logger(ss.str(), true);  // true for incoming message
    }
    
    device_->processInput(group, data);
}

void MidiCISession::log_midi_message_report_chunk(const std::vector<uint8_t>& data) {
    auto logger = device_->get_logger();
    if (logger) {
        std::stringstream ss;
        ss << "[received MIDI (buffered)] ";
        for (uint8_t byte : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        logger(ss.str(), true);
    }
}

void MidiCISession::process_midi1_input(const std::vector<uint8_t>& data, size_t start, size_t length) {
    if (data.size() <= start + 3) return;
    
    // Check for MIDI-CI SysEx message
    if (data[start] == 0xF0 &&
        data[start + 1] == UNIVERSAL_SYSEX &&
        data[start + 3] == SYSEX_SUB_ID_MIDI_CI) {
        
        // Extract CI message (skip F0, include content, skip F7)
        std::vector<uint8_t> ci_data(data.begin() + start + 1, data.begin() + start + length - 1);
        process_ci_message(0, ci_data);  // Group is always 0 for MIDI 1.0
    } else {
        // May be part of MIDI Message Report
        if (receiving_midi_message_reports_) {
            uint8_t channel = data[start] & 0x0F;
            if (channel != last_chunked_message_channel_) {
                if (!chunked_messages_.empty()) {
                    log_midi_message_report_chunk(chunked_messages_);
                }
                chunked_messages_.clear();
                last_chunked_message_channel_ = channel;
            }
            chunked_messages_.insert(chunked_messages_.end(), 
                                   data.begin() + start, data.begin() + start + length);
        } else {
            // Log unexpected MIDI message
            auto logger = device_->get_logger();
            if (logger) {
                std::stringstream ss;
                ss << "[received MIDI1] ";
                for (size_t i = start; i < start + length; ++i) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
                }
                logger(ss.str(), true);
            }
        }
    }
}

void MidiCISession::process_ump_input(const std::vector<uint8_t>& data, size_t start, size_t length) {
    // Parse UMP messages from bytes
    auto umps = ump::parse_umps_from_bytes(data, start, length);
    
    for (const auto& ump : umps) {
        auto msg_type = ump.get_message_type();
        
        if (msg_type == ump::MessageType::SYSEX7) {
            auto status = static_cast<ump::BinaryChunkStatus>(ump.get_status_code());
            
            if (status == ump::BinaryChunkStatus::START) {
                buffered_sysex7_.clear();
            }
            
            // TODO: Extract SysEx7 data when UmpRetriever methods are available
            // For now, this is a placeholder
            
            if (status == ump::BinaryChunkStatus::END || 
                status == ump::BinaryChunkStatus::COMPLETE_PACKET) {
                
                if (buffered_sysex7_.size() > 2 &&
                    buffered_sysex7_[0] == UNIVERSAL_SYSEX &&
                    buffered_sysex7_[2] == SYSEX_SUB_ID_MIDI_CI) {
                    process_ci_message(ump.get_group(), buffered_sysex7_);
                    buffered_sysex7_.clear();
                }
            }
            continue;
        }
        
        if (msg_type == ump::MessageType::SYSEX8_MDS) {
            auto status = static_cast<ump::BinaryChunkStatus>(ump.get_status_code());
            
            if (status == ump::BinaryChunkStatus::START) {
                buffered_sysex8_.clear();
            }
            
            // TODO: Extract SysEx8 data when UmpRetriever methods are available
            
            if (status == ump::BinaryChunkStatus::END || 
                status == ump::BinaryChunkStatus::COMPLETE_PACKET) {
                
                if (buffered_sysex8_.size() > 2 &&
                    buffered_sysex8_[0] == UNIVERSAL_SYSEX &&
                    buffered_sysex8_[2] == SYSEX_SUB_ID_MIDI_CI) {
                    process_ci_message(ump.get_group(), buffered_sysex8_);
                    buffered_sysex8_.clear();
                }
            }
            continue;
        }
        
        // Handle MIDI Message Report for other message types
        if (receiving_midi_message_reports_) {
            // TODO: Extract channel from UMP when method is available
            uint8_t channel = 0;  // Placeholder
            
            if (channel != last_chunked_message_channel_) {
                if (!chunked_messages_.empty()) {
                    log_midi_message_report_chunk(chunked_messages_);
                }
                chunked_messages_.clear();
                last_chunked_message_channel_ = channel;
            }
            
            // TODO: Convert UMP to platform bytes when method is available
            // For now, add raw UMP data
            size_t ump_size = ump.get_size_in_bytes();
            const uint8_t* ump_bytes = reinterpret_cast<const uint8_t*>(&ump);
            chunked_messages_.insert(chunked_messages_.end(), ump_bytes, ump_bytes + ump_size);
        } else {
            // Log unexpected UMP message
            auto logger = device_->get_logger();
            if (logger) {
                std::stringstream ss;
                ss << "[received UMP] ";
                for (size_t i = start; i < start + length; ++i) {
                    ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << " ";
                }
                logger(ss.str(), true);
            }
        }
    }
}

} // namespace midicci::musicdevice