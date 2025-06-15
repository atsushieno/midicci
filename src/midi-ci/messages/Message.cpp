#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"

namespace midi_ci {
namespace messages {

Message::Message(MessageType type, uint32_t source_muid, uint32_t destination_muid)
    : type_(type), source_muid_(source_muid), destination_muid_(destination_muid) {}

MessageType Message::get_type() const noexcept {
    return type_;
}

uint32_t Message::get_source_muid() const noexcept {
    return source_muid_;
}

uint32_t Message::get_destination_muid() const noexcept {
    return destination_muid_;
}

DiscoveryInquiry::DiscoveryInquiry(uint32_t source_muid, uint32_t destination_muid)
    : Message(MessageType::DiscoveryInquiry, source_muid, destination_muid) {}

std::vector<uint8_t> DiscoveryInquiry::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F); // Device ID (broadcast)
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(source_muid_ & 0x7F));
    data.push_back(static_cast<uint8_t>((source_muid_ >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((source_muid_ >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((source_muid_ >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(destination_muid_ & 0x7F));
    data.push_back(static_cast<uint8_t>((destination_muid_ >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((destination_muid_ >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((destination_muid_ >> 21) & 0x7F));
    
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x00);
    
    data.push_back(0x00);
    data.push_back(0x00);
    
    data.push_back(0x00);
    data.push_back(0x00);
    
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x00);
    
    data.push_back(0x00);
    
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x00);
    data.push_back(0x00);
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool DiscoveryInquiry::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 32;
}

} // namespace messages
} // namespace midi_ci
