#include "midicci/midicci.hpp"
#include <umppi/details/UmpFactory.hpp>
#include <umppi/details/UmpRetriever.hpp>
#include <random>
#include <iomanip>
#include <sstream>

namespace midicci::musicdevice {

std::unique_ptr<MidiCISession> createMidiCiSession(
    const MidiCISessionSource& source,
    uint32_t muid,
    MidiCIDeviceConfiguration config,
    MidiCIDevice::LoggerFunction logger
) {
    if (muid == 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(1, 0x7F7F7F7F);
        muid = dis(gen);
    }
    
    auto device = std::make_unique<MidiCIDevice>(muid, config, logger);
    
    // Set up MIDI-CI output sender
    device->setSysexSender([source](uint8_t group, const std::vector<uint8_t>& data) -> bool {
        auto ump_packets = umppi::UmpFactory::sysex7(group, data);

        std::vector<uint32_t> words;
        words.reserve(ump_packets.size() * 4);
        for (const auto& u : ump_packets) {
            size_t base = words.size();
            u.toWords(words, base);
        }

        source.output_sender(umppi::UmpWordSpan{words.data(), words.size()}, 0);
        return true;
    });
    
    return std::make_unique<MidiCISession>(
        source.input_listener_adder,
        std::move(device)
    );
}

MidiCISession::MidiCISession(
    MidiInputListenerAdder input_listener_adder,
    std::unique_ptr<MidiCIDevice> device
) : device_(std::move(device)),
    receiving_midi_message_reports_(false),
    last_chunked_message_channel_(0xFF)  // Invalid channel initially
{
    // Set up MIDI input processing
    input_listener_adder([this](umppi::UmpWordSpan words, uint64_t /*timestamp*/) {
        processUmpInput(words);
    });
    
    // TODO: Set up message received handlers and MIDI message reporter
    // This requires access to device message handling which may need interface additions
}

void MidiCISession::processCiMessage(uint8_t group, const std::vector<uint8_t>& data) {
    if (data.empty()) return;
    
    // Log the message
    auto logger = device_->getLogger();
    if (logger) {
        std::stringstream ss;
        ss << "[received CI SysEx (grp:" << static_cast<int>(group) << ")] ";
        for (uint8_t byte : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        logger(LogData(ss.str(), true));  // true for incoming message
    }
    
    device_->processInput(group, data);
}

void MidiCISession::logMidiMessageReportChunk(const std::vector<uint8_t>& data) {
    auto logger = device_->getLogger();
    if (logger) {
        std::stringstream ss;
        ss << "[received MIDI (buffered)] ";
        for (uint8_t byte : data) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte) << " ";
        }
        logger(LogData(ss.str(), true));
    }
}

void MidiCISession::processUmpInput(umppi::UmpWordSpan words) {
    auto umps = umppi::parseUmpsFromWords(words);
    bool loggedUnexpected = false;
    
    for (const auto& ump : umps) {
        auto msg_type = ump.getMessageType();

        if (msg_type == umppi::MessageType::SYSEX7) {
            auto status = ump.getBinaryChunkStatus();

            if (status == umppi::BinaryChunkStatus::START) {
                buffered_sysex7_.clear();
            }

            umppi::UmpRetriever::DataOutputter outputter{[&](std::vector<uint8_t> data) {
                buffered_sysex7_.insert(buffered_sysex7_.end(), data.begin(), data.end());
            }};
            umppi::UmpRetriever::getSysex7Data(outputter, {ump});

            if (status == umppi::BinaryChunkStatus::END ||
                status == umppi::BinaryChunkStatus::COMPLETE_PACKET) {

                if (buffered_sysex7_.size() > 2 &&
                    buffered_sysex7_[0] == UNIVERSAL_SYSEX &&
                    buffered_sysex7_[2] == SYSEX_SUB_ID_MIDI_CI) {
                    processCiMessage(ump.getGroup(), buffered_sysex7_);
                    buffered_sysex7_.clear();
                }
            }
            continue;
        }

        if (msg_type == umppi::MessageType::SYSEX8_MDS) {
            auto status = ump.getBinaryChunkStatus();

            if (status == umppi::BinaryChunkStatus::START) {
                buffered_sysex8_.clear();
            }

            umppi::UmpRetriever::DataOutputter outputter{[&](std::vector<uint8_t> data) {
                buffered_sysex8_.insert(buffered_sysex8_.end(), data.begin(), data.end());
            }};
            umppi::UmpRetriever::getSysex8Data(outputter, {ump});

            if (status == umppi::BinaryChunkStatus::END ||
                status == umppi::BinaryChunkStatus::COMPLETE_PACKET) {

                if (buffered_sysex8_.size() > 2 &&
                    buffered_sysex8_[0] == UNIVERSAL_SYSEX &&
                    buffered_sysex8_[2] == SYSEX_SUB_ID_MIDI_CI) {
                    processCiMessage(ump.getGroup(), buffered_sysex8_);
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
                    logMidiMessageReportChunk(chunked_messages_);
                }
                chunked_messages_.clear();
                last_chunked_message_channel_ = channel;
            }
            
            auto bytes = ump.toBytes();
            chunked_messages_.insert(chunked_messages_.end(), bytes.begin(), bytes.end());
        } else if (!loggedUnexpected) {
            auto logger = device_->getLogger();
            if (logger) {
                std::stringstream ss;
                ss << "[received UMP] ";
                for (size_t i = 0; i < words.size(); ++i) {
                    ss << std::hex << std::setw(8) << std::setfill('0') << words[i] << " ";
                }
                logger(LogData(ss.str(), true));
            }
            loggedUnexpected = true;
        }
    }
}

} // namespace midicci::musicdevice
