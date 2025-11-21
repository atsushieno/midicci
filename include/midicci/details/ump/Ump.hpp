#pragma once

#include <cstdint>
#include <vector>

namespace midicci {
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
    
    explicit Ump(uint64_t value) : int1(static_cast<uint32_t>(value >> 32)), int2(static_cast<uint32_t>(value & 0xFFFFFFFF)), int3(0), int4(0) {}
    
    // Basic properties
    MessageType get_message_type() const;
    uint8_t get_group() const;
    uint8_t get_status_code() const;
    uint8_t get_status_byte() const;
    BinaryChunkStatus get_binary_chunk_status() const;
    uint8_t get_sysex7_size() const;
    uint8_t get_sysex8_size() const;
    size_t get_size_in_bytes() const;
    
    // MIDI1 accessors
    uint8_t get_midi1_msb() const;
    uint8_t get_midi1_lsb() const;
    uint8_t get_channel_in_group() const;
    
    // MIDI2 accessors
    uint8_t get_midi2_note() const;
    uint16_t get_midi2_velocity16() const;
    uint32_t get_midi2_paf_data() const;
    uint8_t get_midi2_cc_index() const;
    uint32_t get_midi2_cc_data() const;
    uint8_t get_midi2_program_options() const;
    uint8_t get_midi2_program_program() const;
    uint8_t get_midi2_program_bank_msb() const;
    uint8_t get_midi2_program_bank_lsb() const;
    uint32_t get_midi2_caf_data() const;
    uint32_t get_midi2_pitch_bend_data() const;
    uint8_t get_midi2_rpn_msb() const;
    uint8_t get_midi2_rpn_lsb() const;
    uint32_t get_midi2_rpn_data() const;
    uint8_t get_midi2_nrpn_msb() const;
    uint8_t get_midi2_nrpn_lsb() const;
    uint32_t get_midi2_nrpn_data() const;
    
    // Timing accessors
    bool is_delta_clockstamp() const;
    bool is_jr_timestamp() const;
    bool is_dctpq() const;
    bool is_start_of_clip() const;
    bool is_end_of_clip() const;
    uint32_t get_delta_clockstamp() const;
    uint16_t get_jr_timestamp() const;
    
    // Data conversion
    std::vector<uint8_t> to_platform_bytes() const;
    static std::vector<Ump> from_bytes(const std::vector<uint8_t>& bytes, size_t start, size_t length);
};

std::vector<Ump> parse_umps_from_bytes(const uint8_t* data, size_t start, size_t length);

} // namespace ump
} // namespace midi_ci
