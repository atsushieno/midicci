#include <umppi/details/UmpFactory.hpp>
#include <algorithm>
#include <stdexcept>

namespace umppi {

uint32_t UmpFactory::noop() {
    return 0;
}

uint32_t UmpFactory::jrClock(uint16_t senderClockTime16) {
    return (0x10 << 16) + senderClockTime16;
}

uint32_t UmpFactory::jrClock(double senderClockTimeSeconds) {
    uint16_t value = static_cast<uint16_t>(senderClockTimeSeconds * JR_TIMESTAMP_TICKS_PER_SECOND);
    return jrClock(value);
}

uint32_t UmpFactory::jrTimestamp(uint16_t senderClockTimestamp16) {
    if (senderClockTimestamp16 > 0xFFFF)
        throw std::invalid_argument("Timestamp value must be less than 65536");
    return (0x20 << 16) + senderClockTimestamp16;
}

uint32_t UmpFactory::jrTimestamp(double senderClockTimestampSeconds) {
    return jrTimestamp(static_cast<uint16_t>(senderClockTimestampSeconds * JR_TIMESTAMP_TICKS_PER_SECOND));
}

uint32_t UmpFactory::dctpq(uint16_t numberOfTicksPerQuarterNote) {
    return (0x30 << 16) + numberOfTicksPerQuarterNote;
}

uint32_t UmpFactory::deltaClockstamp(uint32_t ticks20) {
    return (0x40 << 16) + (ticks20 & 0xFFFFF);
}

uint32_t UmpFactory::systemMessage(uint8_t group, uint8_t status, uint8_t midi1Byte2, uint8_t midi1Byte3) {
    return (static_cast<uint32_t>(MessageType::SYSTEM) << 28) +
           ((group & 0xF) << 24) +
           (status << 16) +
           ((midi1Byte2 & 0x7F) << 8) +
           (midi1Byte3 & 0x7F);
}

uint32_t UmpFactory::midi1Message(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4) {
    return (static_cast<uint32_t>(MessageType::MIDI1) << 28) +
           ((group & 0xF) << 24) +
           (((code & 0xF0) + (channel & 0xF)) << 16) +
           ((byte3 & 0x7F) << 8) +
           (byte4 & 0x7F);
}

uint32_t UmpFactory::midi1NoteOff(uint8_t group, uint8_t channel, uint8_t note, uint8_t velocity) {
    return midi1Message(group, MidiChannelStatus::NOTE_OFF, channel, note & 0x7F, velocity & 0x7F);
}

uint32_t UmpFactory::midi1NoteOn(uint8_t group, uint8_t channel, uint8_t note, uint8_t velocity) {
    return midi1Message(group, MidiChannelStatus::NOTE_ON, channel, note & 0x7F, velocity & 0x7F);
}

uint32_t UmpFactory::midi1PAf(uint8_t group, uint8_t channel, uint8_t note, uint8_t data) {
    return midi1Message(group, MidiChannelStatus::PAF, channel, note & 0x7F, data & 0x7F);
}

uint32_t UmpFactory::midi1CC(uint8_t group, uint8_t channel, uint8_t index, uint8_t data) {
    return midi1Message(group, MidiChannelStatus::CC, channel, index & 0x7F, data & 0x7F);
}

uint32_t UmpFactory::midi1Program(uint8_t group, uint8_t channel, uint8_t program) {
    return midi1Message(group, MidiChannelStatus::PROGRAM, channel, program & 0x7F, 0);
}

uint32_t UmpFactory::midi1CAf(uint8_t group, uint8_t channel, uint8_t data) {
    return midi1Message(group, MidiChannelStatus::CAF, channel, data & 0x7F, 0);
}

uint32_t UmpFactory::midi1PitchBendDirect(uint8_t group, uint8_t channel, uint16_t data14) {
    return midi1Message(group, MidiChannelStatus::PITCH_BEND, channel, data14 & 0x7F, (data14 >> 7) & 0x7F);
}

uint32_t UmpFactory::midi1PitchBend(uint8_t group, uint8_t channel, int16_t data) {
    uint16_t unsigned_data = static_cast<uint16_t>(data + 8192);
    return midi1PitchBendDirect(group, channel, unsigned_data);
}

uint64_t UmpFactory::midi2ChannelMessage8_8_16_16(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4, uint16_t short1, uint16_t short2) {
    uint64_t int1 = (static_cast<uint64_t>(MessageType::MIDI2) << 28) +
                    ((group & 0xF) << 24) +
                    (((code & 0xF0) + (channel & 0xF)) << 16) +
                    (byte3 << 8) + byte4;
    uint32_t int2 = ((short1 & 0xFFFF) << 16) + (short2 & 0xFFFF);
    return (int1 << 32) + int2;
}

uint64_t UmpFactory::midi2ChannelMessage8_8_32(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4, uint32_t rest32) {
    uint64_t int1 = (static_cast<uint64_t>(MessageType::MIDI2) << 28) +
                    ((group & 0xF) << 24) +
                    (((code & 0xF0) + (channel & 0xF)) << 16) +
                    (byte3 << 8) + byte4;
    return (int1 << 32) + rest32;
}

uint16_t UmpFactory::pitch7_9(double pitch) {
    double actual = (pitch < 0.0) ? 0.0 : (pitch >= 128.0) ? 128.0 : pitch;
    uint8_t semitone = static_cast<uint8_t>(actual);
    double microtone = actual - semitone;
    return (semitone << 9) + static_cast<uint16_t>(microtone * 512.0);
}

uint16_t UmpFactory::pitch7_9Split(uint8_t semitone, double microtone0To1) {
    uint16_t ret = (semitone & 0x7F) << 9;
    double actual = (microtone0To1 < 0.0) ? 0.0 : (microtone0To1 > 1.0) ? 1.0 : microtone0To1;
    ret += static_cast<uint16_t>(actual * 512.0);
    return ret;
}

uint64_t UmpFactory::midi2NoteOff(uint8_t group, uint8_t channel, uint8_t note, uint8_t attributeType8, uint16_t velocity16, uint16_t attributeData16) {
    return midi2ChannelMessage8_8_16_16(group, MidiChannelStatus::NOTE_OFF, channel, note & 0x7F, attributeType8, velocity16, attributeData16);
}

uint64_t UmpFactory::midi2NoteOn(uint8_t group, uint8_t channel, uint8_t note, uint8_t attributeType8, uint16_t velocity16, uint16_t attributeData16) {
    return midi2ChannelMessage8_8_16_16(group, MidiChannelStatus::NOTE_ON, channel, note & 0x7F, attributeType8, velocity16, attributeData16);
}

uint64_t UmpFactory::midi2PAf(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PAF, channel, note & 0x7F, MIDI_2_0_RESERVED, data32);
}

uint64_t UmpFactory::midi2CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::CC, channel, index, MIDI_2_0_RESERVED, data32);
}

uint64_t UmpFactory::midi2Program(uint8_t group, uint8_t channel, uint8_t options, uint8_t program, uint8_t bankMsb, uint8_t bankLsb) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PROGRAM, channel, MIDI_2_0_RESERVED, options & 1,
                                     ((program & 0x7F) << 24) + (static_cast<uint32_t>(bankMsb) << 8) + bankLsb);
}

uint64_t UmpFactory::midi2CAf(uint8_t group, uint8_t channel, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::CAF, channel, MIDI_2_0_RESERVED, MIDI_2_0_RESERVED, data32);
}

uint64_t UmpFactory::midi2PitchBendDirect(uint8_t group, uint8_t channel, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PITCH_BEND, channel, MIDI_2_0_RESERVED, MIDI_2_0_RESERVED, data32);
}

uint64_t UmpFactory::midi2PitchBend(uint8_t group, uint8_t channel, int32_t data) {
    return midi2PitchBendDirect(group, channel, 0x80000000U + data);
}

uint64_t UmpFactory::midi2RPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::RPN, channel, msb, lsb, data32);
}

uint64_t UmpFactory::midi2NRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::NRPN, channel, msb, lsb, data32);
}

Ump UmpFactory::sysex7Direct(uint8_t group, uint8_t status, uint8_t numBytes,
                              uint8_t data1, uint8_t data2, uint8_t data3,
                              uint8_t data4, uint8_t data5, uint8_t data6) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::SYSEX7) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    (static_cast<uint32_t>(status + numBytes) << 16) |
                    (static_cast<uint32_t>(data1) << 8) |
                    static_cast<uint32_t>(data2);

    uint32_t int2 = (static_cast<uint32_t>(data3) << 24) |
                    (static_cast<uint32_t>(data4) << 16) |
                    (static_cast<uint32_t>(data5) << 8) |
                    static_cast<uint32_t>(data6);

    return Ump(int1, int2);
}

int UmpFactory::sysex7GetSysexLength(const std::vector<uint8_t>& src_data) {
    int i = 0;
    while (i < static_cast<int>(src_data.size()) && src_data[i] != 0xF7) {
        i++;
    }
    return i - (src_data.size() > 0 && src_data[0] == 0xF0 ? 1 : 0);
}

int UmpFactory::sysex7GetPacketCount(const std::vector<uint8_t>& src_data) {
    int length = sysex7GetSysexLength(src_data);
    return (length + SYSEX7_RADIX - 1) / SYSEX7_RADIX;
}

Ump UmpFactory::sysex7GetPacketOf(uint8_t group, const std::vector<uint8_t>& src_data, int packet_index) {
    return sysexGetPacketOf(MessageType::SYSEX7, group, src_data, packet_index, SYSEX7_RADIX, false, 0);
}

void UmpFactory::sysex7Process(uint8_t group, const std::vector<uint8_t>& src_data,
                                std::function<void(const Ump&)> callback) {
    int packet_count = sysex7GetPacketCount(src_data);
    for (int i = 0; i < packet_count; i++) {
        callback(sysex7GetPacketOf(group, src_data, i));
    }
}

std::vector<Ump> UmpFactory::sysex7(uint8_t group, const std::vector<uint8_t>& src_data) {
    std::vector<Ump> result;
    sysex7Process(group, src_data, [&result](const Ump& ump) {
        result.push_back(ump);
    });
    return result;
}

Ump UmpFactory::sysexGetPacketOf(MessageType message_type, uint8_t group,
                                    const std::vector<uint8_t>& src_data, int packet_index, int radix, bool hasStreamId, uint8_t streamId) {
    int sysex_length = sysex7GetSysexLength(src_data);
    int packet_count = (sysex_length + radix - 1) / radix;

    BinaryChunkStatus status;
    if (packet_count == 1) {
        status = BinaryChunkStatus::COMPLETE_PACKET;
    } else if (packet_index == 0) {
        status = BinaryChunkStatus::START;
    } else if (packet_index == packet_count - 1) {
        status = BinaryChunkStatus::END;
    } else {
        status = BinaryChunkStatus::CONTINUE;
    }

    int data_start = src_data.size() > 0 && src_data[0] == 0xF0 ? 1 : 0;
    int data_pos = data_start + packet_index * radix;

    int remaining_bytes = sysex_length - packet_index * radix;
    int packet_bytes = std::min(remaining_bytes, radix);

    uint32_t int1 = (static_cast<uint32_t>(message_type) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    (static_cast<uint32_t>(status) << 16) |
                    (static_cast<uint32_t>(packet_bytes & 0xF) << 16);
    if (packet_bytes > 0)
        int1 |= src_data[data_pos] << 8;
    if (packet_bytes > 1)
        int1 |= src_data[data_pos + 1];

    uint32_t int2 = 0;
    uint8_t start = hasStreamId ? 1 : 2;
    for (int i = start; i < packet_bytes && i < start + 4; i++) {
        if (data_pos + i < static_cast<int>(src_data.size())) {
            int2 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - (i - start) * 8);
        }
    }

    if (message_type == MessageType::SYSEX7) {
        return Ump(int1, int2);
    }

    uint32_t int3 = 0;
    uint32_t int4 = 0;
    start = hasStreamId ? 5 : 6;
    for (int i = start; i < packet_bytes && i < radix; i++) {
        if (data_pos + i < static_cast<int>(src_data.size())) {
            if (i < start + 4) {
                int3 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - (i - start) * 8);
            } else {
                int4 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - (i - start - 4) * 8);
            }
        }
    }

    return Ump(int1, int2, int3, int4);
}

Ump UmpFactory::startOfClip() {
    return Ump(0xF0200000, 0, 0, 0);
}

Ump UmpFactory::endOfClip() {
    return Ump(0xF0210000, 0, 0, 0);
}

} // namespace umppi
