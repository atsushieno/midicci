#pragma once

#include <cstdint>
#include <vector>

namespace midi_ci {
namespace ump {

enum class MessageType : uint8_t {
    UTILITY = 0,
    SYSTEM = 1,
    MIDI1 = 2,
    SYSEX7 = 3,
    MIDI2 = 4,
    SYSEX8_MDS = 5,
    FLEX_DATA = 0xD,
    UMP_STREAM = 0xF
};

enum class BinaryChunkStatus : uint8_t {
    COMPLETE_PACKET = 0,
    START = 0x10,
    CONTINUE = 0x20,
    END = 0x30
};

struct Ump {
    uint32_t int1;
    uint32_t int2;
    uint32_t int3;
    uint32_t int4;
    
    Ump(uint32_t i1, uint32_t i2 = 0, uint32_t i3 = 0, uint32_t i4 = 0)
        : int1(i1), int2(i2), int3(i3), int4(i4) {}
    
    MessageType get_message_type() const;
    uint8_t get_group() const;
    uint8_t get_status_code() const;
    uint8_t get_sysex7_size() const;
    uint8_t get_sysex8_size() const;
    size_t get_size_in_bytes() const;
};

std::vector<Ump> parse_umps_from_bytes(const std::vector<uint8_t>& data, size_t start, size_t length);

} // namespace ump
} // namespace midi_ci
