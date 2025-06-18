#include "midicci/ump/Ump.hpp"
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

static uint32_t get_int_from_bytes(const std::vector<uint8_t>& bytes, size_t offset) {
    if (offset + 3 >= bytes.size()) return 0;
    
    return bytes[offset] |
           (bytes[offset + 1] << 8) |
           (bytes[offset + 2] << 16) |
           (bytes[offset + 3] << 24);
}

std::vector<Ump> parse_umps_from_bytes(const std::vector<uint8_t>& data, size_t start, size_t length) {
    std::vector<Ump> result;
    size_t offset = start;
    const size_t end = start + length;
    
    while (offset < end && offset + 3 < data.size()) {
        uint32_t int1 = get_int_from_bytes(data, offset);
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
        
        uint32_t int2 = (ump_size > 4) ? get_int_from_bytes(data, offset + 4) : 0;
        uint32_t int3 = (ump_size > 8) ? get_int_from_bytes(data, offset + 8) : 0;
        uint32_t int4 = (ump_size > 12) ? get_int_from_bytes(data, offset + 12) : 0;
        
        result.emplace_back(int1, int2, int3, int4);
        offset += ump_size;
    }
    
    return result;
}

} // namespace ump
} // namespace midi_ci
