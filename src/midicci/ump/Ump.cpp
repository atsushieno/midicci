#include "midicci/midicci.hpp"
#include <cstring>

namespace midicci {
namespace ump {

MessageType Ump::get_message_type() const {
    return static_cast<MessageType>((int1 >> 28) & 0xF);
}

uint8_t Ump::get_group() const {
    return (int1 >> 24) & 0xF;
}

uint8_t Ump::get_status_code() const {
    return ((int1 >> 16) & 0xFF) & 0xF0;
}

uint8_t Ump::get_status_byte() const {
    return (int1 >> 16) & 0xFF;
}

BinaryChunkStatus Ump::get_binary_chunk_status() const {
    uint8_t status = get_status_byte();
    // For sysex messages, the binary chunk status is in specific bit patterns
    if (status == 0x00) return BinaryChunkStatus::COMPLETE_PACKET;
    if (status == 0x10) return BinaryChunkStatus::START;
    if (status == 0x20) return BinaryChunkStatus::CONTINUE;
    if (status == 0x30) return BinaryChunkStatus::END;
    return BinaryChunkStatus::COMPLETE_PACKET; // Default fallback
}

uint8_t Ump::get_sysex7_size() const {
    return (int1 >> 16) & 0xF;
}

uint8_t Ump::get_sysex8_size() const {
    return (int1 >> 16) & 0xF;
}

size_t Ump::get_size_in_bytes() const {
    switch (get_message_type()) {
        case MessageType::SYSEX8_MDS:
        case MessageType::FLEX_DATA:
        case MessageType::UMP_STREAM:
            return 16;
        case MessageType::SYSEX7:
        case MessageType::MIDI2:
            return 8;
        default:
            return 4;
    }
}

// MIDI1 accessors
uint8_t Ump::get_midi1_msb() const {
    return (int1 >> 8) & 0x7F;
}

uint8_t Ump::get_midi1_lsb() const {
    return int1 & 0x7F;
}

uint8_t Ump::get_channel_in_group() const {
    return (int1 >> 16) & 0xF;
}

// MIDI2 accessors
uint8_t Ump::get_midi2_note() const {
    return (int1 >> 8) & 0x7F;
}

uint16_t Ump::get_midi2_velocity16() const {
    return (int2 >> 16) & 0xFFFF;
}

uint32_t Ump::get_midi2_paf_data() const {
    return int2;
}

uint8_t Ump::get_midi2_cc_index() const {
    return (int1 >> 8) & 0x7F;
}

uint32_t Ump::get_midi2_cc_data() const {
    return int2;
}

uint8_t Ump::get_midi2_program_options() const {
    return int1 & 0x1;
}

uint8_t Ump::get_midi2_program_program() const {
    return (int2 >> 24) & 0x7F;
}

uint8_t Ump::get_midi2_program_bank_msb() const {
    return (int2 >> 8) & 0x7F;
}

uint8_t Ump::get_midi2_program_bank_lsb() const {
    return int2 & 0x7F;
}

uint32_t Ump::get_midi2_caf_data() const {
    return int2;
}

uint32_t Ump::get_midi2_pitch_bend_data() const {
    return int2;
}

uint8_t Ump::get_midi2_rpn_msb() const {
    return (int1 >> 8) & 0x7F;
}

uint8_t Ump::get_midi2_rpn_lsb() const {
    return int1 & 0x7F;
}

uint32_t Ump::get_midi2_rpn_data() const {
    return int2;
}

uint8_t Ump::get_midi2_nrpn_msb() const {
    return (int1 >> 8) & 0x7F;
}

uint8_t Ump::get_midi2_nrpn_lsb() const {
    return int1 & 0x7F;
}

uint32_t Ump::get_midi2_nrpn_data() const {
    return int2;
}

// Timing accessors
bool Ump::is_delta_clockstamp() const {
    return get_message_type() == MessageType::UTILITY && get_status_byte() == 0x40;
}

bool Ump::is_jr_timestamp() const {
    return get_message_type() == MessageType::UTILITY && get_status_byte() == 0x20;
}

bool Ump::is_dctpq() const {
    return get_message_type() == MessageType::UTILITY && get_status_byte() == 0x30;
}

bool Ump::is_start_of_clip() const {
    return get_message_type() == MessageType::UMP_STREAM && get_status_byte() == 0x20;
}

bool Ump::is_end_of_clip() const {
    return get_message_type() == MessageType::UMP_STREAM && get_status_byte() == 0x21;
}

uint32_t Ump::get_delta_clockstamp() const {
    return int1 & 0xFFFFF; // 20 bits
}

uint16_t Ump::get_jr_timestamp() const {
    return int1 & 0xFFFF;
}

static uint32_t get_int_from_bytes(const uint8_t* bytes, size_t offset, size_t max_size) {
    if (offset + 3 >= max_size) return 0;
    
    return bytes[offset] |
           (bytes[offset + 1] << 8) |
           (bytes[offset + 2] << 16) |
           (bytes[offset + 3] << 24);
}

std::vector<Ump> parse_umps_from_bytes(const uint8_t* data, size_t start, size_t length) {
    std::vector<Ump> result;
    size_t offset = start;
    const size_t end = start + length;
    
    while (offset < end && (offset - start + 3) < length) {
        uint32_t int1 = get_int_from_bytes(data, offset, start + length);
        MessageType msg_type = static_cast<MessageType>((int1 >> 28) & 0xF);
        
        size_t ump_size;
        switch (msg_type) {
            case MessageType::SYSEX8_MDS:
            case MessageType::FLEX_DATA:
            case MessageType::UMP_STREAM:
                ump_size = 16;
                break;
            case MessageType::SYSEX7:
            case MessageType::MIDI2:
                ump_size = 8;
                break;
            default:
                ump_size = 4;
                break;
        }
        
        if (offset + ump_size > end) break;
        
        uint32_t int2 = (ump_size > 4) ? get_int_from_bytes(data, offset + 4, start + length) : 0;
        uint32_t int3 = (ump_size > 8) ? get_int_from_bytes(data, offset + 8, start + length) : 0;
        uint32_t int4 = (ump_size > 12) ? get_int_from_bytes(data, offset + 12, start + length) : 0;
        
        result.emplace_back(int1, int2, int3, int4);
        offset += ump_size;
    }
    
    return result;
}

} // namespace ump
} // namespace midi_ci
