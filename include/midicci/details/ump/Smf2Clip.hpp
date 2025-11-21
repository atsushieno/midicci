#pragma once

#include "midicci/details/ump/Ump.hpp"
#include "midicci/details/ump/UmpFactory.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <cstdint>

namespace midicci {
namespace ump {

// MIDI Clip File per M2-116-U specification
// File structure:
//   1. File Header (8 bytes: "SMF2CLIP")
//   2. Clip Configuration Header (UMP messages with DCS)
//   3. Clip Sequence Data (UMP messages with DCS, starts with Start of Clip, ends with End of Clip)

class MidiClipFile {
public:
    std::vector<Ump> configuration_messages;  // Messages in Clip Configuration Header
    std::vector<Ump> sequence_messages;       // Messages in Clip Sequence Data
    uint16_t ticks_per_quarter_note;          // DCTPQ value

    MidiClipFile(uint16_t tpqn = 480) : ticks_per_quarter_note(tpqn) {}

    // Write to a file (default extension: .midi2)
    void write_to_file(const std::string& filename) const;

    // Read from a file
    static MidiClipFile read_from_file(const std::string& filename);

    // Write to a byte vector
    std::vector<uint8_t> to_bytes() const;

    // Read from a byte vector
    static MidiClipFile from_bytes(const std::vector<uint8_t>& data);

    // Helper: Add a configuration message with DCS
    void add_config_message(uint32_t delta_ticks, const Ump& ump);

    // Helper: Add a sequence message with DCS
    void add_sequence_message(uint32_t delta_ticks, const Ump& ump);

private:
    static void write_int32_big_endian(std::vector<uint8_t>& buffer, uint32_t value);
    static uint32_t read_int32_big_endian(const uint8_t* data, size_t& offset);
    static void write_ump(std::vector<uint8_t>& buffer, const Ump& ump);
    static Ump read_ump(const uint8_t* data, size_t& offset, size_t max_size);
};

} // namespace ump
} // namespace midicci
