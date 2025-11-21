#include "midicci/details/ump/Smf2Clip.hpp"
#include <stdexcept>
#include <cstring>

namespace midicci {
namespace ump {

// MIDI Clip File per M2-116-U MIDI Clip File Specification v1.0

void MidiClipFile::write_int32_big_endian(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back((value >> 24) & 0xFF);
    buffer.push_back((value >> 16) & 0xFF);
    buffer.push_back((value >> 8) & 0xFF);
    buffer.push_back(value & 0xFF);
}

uint32_t MidiClipFile::read_int32_big_endian(const uint8_t* data, size_t& offset) {
    uint32_t result = (static_cast<uint32_t>(data[offset]) << 24) |
                      (static_cast<uint32_t>(data[offset + 1]) << 16) |
                      (static_cast<uint32_t>(data[offset + 2]) << 8) |
                      static_cast<uint32_t>(data[offset + 3]);
    offset += 4;
    return result;
}

void MidiClipFile::write_ump(std::vector<uint8_t>& buffer, const Ump& ump) {
    write_int32_big_endian(buffer, ump.int1);

    MessageType msg_type = ump.get_message_type();
    if (msg_type == MessageType::SYSEX8_MDS ||
        msg_type == MessageType::FLEX_DATA ||
        msg_type == MessageType::UMP_STREAM) {
        // 16-byte message (4 words)
        write_int32_big_endian(buffer, ump.int2);
        write_int32_big_endian(buffer, ump.int3);
        write_int32_big_endian(buffer, ump.int4);
    } else if (msg_type == MessageType::SYSEX7 || msg_type == MessageType::MIDI2) {
        // 8-byte message (2 words)
        write_int32_big_endian(buffer, ump.int2);
    }
    // 4-byte messages (1 word) - only int1 is written
}

Ump MidiClipFile::read_ump(const uint8_t* data, size_t& offset, size_t max_size) {
    if (offset + 4 > max_size) {
        throw std::runtime_error("Insufficient data for UMP at offset " + std::to_string(offset));
    }

    uint32_t int1 = read_int32_big_endian(data, offset);
    MessageType msg_type = static_cast<MessageType>((int1 >> 28) & 0xF);

    // Read additional words based on message type
    if (msg_type == MessageType::SYSEX8_MDS ||
        msg_type == MessageType::FLEX_DATA ||
        msg_type == MessageType::UMP_STREAM) {
        // 16-byte message
        if (offset + 12 > max_size) {
            throw std::runtime_error("Insufficient data for 16-byte UMP");
        }
        uint32_t int2 = read_int32_big_endian(data, offset);
        uint32_t int3 = read_int32_big_endian(data, offset);
        uint32_t int4 = read_int32_big_endian(data, offset);
        return Ump(int1, int2, int3, int4);
    } else if (msg_type == MessageType::SYSEX7 || msg_type == MessageType::MIDI2) {
        // 8-byte message
        if (offset + 4 > max_size) {
            throw std::runtime_error("Insufficient data for 8-byte UMP");
        }
        uint32_t int2 = read_int32_big_endian(data, offset);
        return Ump(int1, int2);
    }

    // 4-byte message
    return Ump(int1);
}

void MidiClipFile::add_config_message(uint32_t delta_ticks, const Ump& ump) {
    configuration_messages.push_back(Ump(UmpFactory::deltaClockstamp(delta_ticks)));
    configuration_messages.push_back(ump);
}

void MidiClipFile::add_sequence_message(uint32_t delta_ticks, const Ump& ump) {
    sequence_messages.push_back(Ump(UmpFactory::deltaClockstamp(delta_ticks)));
    sequence_messages.push_back(ump);
}

std::vector<uint8_t> MidiClipFile::to_bytes() const {
    std::vector<uint8_t> result;

    // 1. File Header: "SMF2CLIP" (8 bytes)
    result.push_back('S');
    result.push_back('M');
    result.push_back('F');
    result.push_back('2');
    result.push_back('C');
    result.push_back('L');
    result.push_back('I');
    result.push_back('P');

    // 2. Clip Configuration Header
    // First: DCS(0) + DCTPQ message (mandatory per spec section 6)
    write_int32_big_endian(result, UmpFactory::deltaClockstamp(0));
    write_int32_big_endian(result, UmpFactory::dctpq(ticks_per_quarter_note));

    // Add configuration messages (already include their DCS)
    for (const auto& ump : configuration_messages) {
        write_ump(result, ump);
    }

    // 3. Clip Sequence Data
    // Must start with: DCS + Start of Clip
    if (sequence_messages.empty() || !sequence_messages[1].is_start_of_clip()) {
        // Auto-add Start of Clip if not present
        write_int32_big_endian(result, UmpFactory::deltaClockstamp(0));
        write_ump(result, UmpFactory::startOfClip());
    }

    // Add sequence messages (already include their DCS)
    for (const auto& ump : sequence_messages) {
        write_ump(result, ump);
    }

    // Must end with: DCS + End of Clip
    bool has_end_of_clip = false;
    if (!sequence_messages.empty()) {
        for (size_t i = 0; i < sequence_messages.size(); ++i) {
            if (sequence_messages[i].is_end_of_clip()) {
                has_end_of_clip = true;
                break;
            }
        }
    }

    if (!has_end_of_clip) {
        // Auto-add End of Clip if not present
        write_int32_big_endian(result, UmpFactory::deltaClockstamp(0));
        write_ump(result, UmpFactory::endOfClip());
    }

    return result;
}

MidiClipFile MidiClipFile::from_bytes(const std::vector<uint8_t>& data) {
    if (data.size() < 8) {
        throw std::runtime_error("Insufficient data for MIDI Clip File header");
    }

    size_t offset = 0;

    // 1. Verify File Header: "SMF2CLIP"
    const char expected_header[] = "SMF2CLIP";
    for (int i = 0; i < 8; ++i) {
        if (data[offset++] != static_cast<uint8_t>(expected_header[i])) {
            throw std::runtime_error("Invalid MIDI Clip File header - expected 'SMF2CLIP'");
        }
    }

    // 2. Read Clip Configuration Header
    // First UMP should be DCS(0)
    Ump first_dcs = read_ump(data.data(), offset, data.size());
    if (!first_dcs.is_delta_clockstamp()) {
        throw std::runtime_error("Expected Delta Clockstamp after file header");
    }

    // Second UMP should be DCTPQ
    Ump dctpq_ump = read_ump(data.data(), offset, data.size());
    if (!dctpq_ump.is_dctpq()) {
        throw std::runtime_error("Expected DCTPQ message after initial Delta Clockstamp");
    }

    uint16_t tpqn = dctpq_ump.int1 & 0xFFFF;
    MidiClipFile result(tpqn);

    // Read configuration messages until we hit Start of Clip
    while (offset < data.size()) {
        Ump ump = read_ump(data.data(), offset, data.size());

        // Start of Clip marks the end of configuration header
        if (ump.is_start_of_clip()) {
            // This Start of Clip belongs to sequence data
            // Need to read its preceding DCS too
            // Go back and re-read from the DCS before Start of Clip
            // Actually, we already read the DCS as part of config messages
            // So we need to handle this differently

            // The last message in configuration_messages should be the DCS for Start of Clip
            // Move it to sequence_messages
            if (!result.configuration_messages.empty()) {
                Ump last_msg = result.configuration_messages.back();
                result.configuration_messages.pop_back();
                result.sequence_messages.push_back(last_msg);  // The DCS
            }
            result.sequence_messages.push_back(ump);  // The Start of Clip
            break;
        }

        result.configuration_messages.push_back(ump);
    }

    // 3. Read Clip Sequence Data until End of Clip
    while (offset < data.size()) {
        Ump ump = read_ump(data.data(), offset, data.size());
        result.sequence_messages.push_back(ump);

        if (ump.is_end_of_clip()) {
            break;
        }
    }

    return result;
}

void MidiClipFile::write_to_file(const std::string& filename) const {
    std::vector<uint8_t> bytes = to_bytes();
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    file.close();
}

MidiClipFile MidiClipFile::read_from_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading: " + filename);
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        throw std::runtime_error("Failed to read file: " + filename);
    }

    return from_bytes(buffer);
}

} // namespace ump
} // namespace midicci
