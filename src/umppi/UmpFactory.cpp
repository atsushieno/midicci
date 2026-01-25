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

std::vector<uint32_t> UmpFactory::jrTimestamps(uint64_t senderClockTimestampTicks) {
    std::vector<uint32_t> result;
    uint64_t remaining = senderClockTimestampTicks;

    while (remaining > 0xFFFF) {
        result.push_back(jrTimestamp(static_cast<uint16_t>(0xFFFF)));
        remaining -= 0x10000;
    }

    result.push_back(jrTimestamp(static_cast<uint16_t>(remaining)));
    return result;
}

std::vector<uint32_t> UmpFactory::jrTimestamps(double senderClockTimestampSeconds) {
    return jrTimestamps(static_cast<uint64_t>(senderClockTimestampSeconds * JR_TIMESTAMP_TICKS_PER_SECOND));
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

uint32_t UmpFactory::midi1PitchBendSplit(uint8_t group, uint8_t channel, uint8_t dataLSB, uint8_t dataMSB) {
    uint16_t data14 = (dataLSB & 0x7F) | ((dataMSB & 0x7F) << 7);
    return midi1PitchBendDirect(group, channel, data14);
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

uint64_t UmpFactory::midi2RelativeRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::RELATIVE_RPN, channel, msb, lsb, data32);
}

uint64_t UmpFactory::midi2RelativeNRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::RELATIVE_NRPN, channel, msb, lsb, data32);
}

uint64_t UmpFactory::midi2PerNoteRCC(uint8_t group, uint8_t channel, uint8_t note, uint8_t index, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PER_NOTE_RCC, channel, note & 0x7F, index, data32);
}

uint64_t UmpFactory::midi2PerNoteACC(uint8_t group, uint8_t channel, uint8_t note, uint8_t index, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PER_NOTE_ACC, channel, note & 0x7F, index, data32);
}

uint64_t UmpFactory::midi2PerNoteManagement(uint8_t group, uint8_t channel, uint8_t note, uint8_t optionFlags) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PER_NOTE_MANAGEMENT, channel, note & 0x7F, optionFlags, 0);
}

uint64_t UmpFactory::midi2PerNotePitchBend(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32) {
    return midi2PerNotePitchBendDirect(group, channel, note, 0x80000000U + data32);
}

uint64_t UmpFactory::midi2PerNotePitchBendDirect(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32) {
    return midi2ChannelMessage8_8_32(group, MidiChannelStatus::PER_NOTE_PITCH_BEND, channel, note & 0x7F, MIDI_2_0_RESERVED, data32);
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

int UmpFactory::getPacketCountCommon(int numBytes, int radix) {
    if (numBytes == 0) return 1;
    return (numBytes + radix - 1) / radix;
}

int UmpFactory::sysex8GetPacketCount(int numBytes) {
    return getPacketCountCommon(numBytes, SYSEX8_RADIX);
}

Ump UmpFactory::sysex8GetPacketOf(uint8_t group, uint8_t streamId, const std::vector<uint8_t>& src_data, int packet_index) {
    return sysexGetPacketOf(MessageType::SYSEX8_MDS, group, src_data, packet_index, SYSEX8_RADIX, true, streamId);
}

void UmpFactory::sysex8Process(uint8_t group, const std::vector<uint8_t>& src_data, uint8_t streamId,
                                std::function<void(const Ump&)> callback) {
    int packet_count = sysex8GetPacketCount(src_data.size());
    for (int i = 0; i < packet_count; i++) {
        callback(sysex8GetPacketOf(group, streamId, src_data, i));
    }
}

std::vector<Ump> UmpFactory::sysex8(uint8_t group, const std::vector<uint8_t>& src_data, uint8_t streamId) {
    std::vector<Ump> result;
    sysex8Process(group, src_data, streamId, [&result](const Ump& ump) {
        result.push_back(ump);
    });
    return result;
}

int UmpFactory::mdsGetChunkCount(int numTotalBytesInMDS) {
    constexpr int MDS_CHUNK_SIZE = 14 * 0x10000;
    return (numTotalBytesInMDS + MDS_CHUNK_SIZE - 1) / MDS_CHUNK_SIZE;
}

int UmpFactory::mdsGetPayloadCount(int numTotalBytesInChunk) {
    constexpr int MDS_PAYLOAD_SIZE = 14;
    return (numTotalBytesInChunk + MDS_PAYLOAD_SIZE - 1) / MDS_PAYLOAD_SIZE;
}

Ump UmpFactory::mdsGetHeader(uint8_t group, uint8_t mdsId, uint16_t numBytesInChunk,
                              uint16_t numChunks, uint16_t chunkIndex, uint16_t manufacturerId,
                              uint16_t deviceId, uint16_t subId, uint16_t subId2) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::SYSEX8_MDS) << 28) |
                    ((group & 0xF) << 24) |
                    (Midi2BinaryChunkStatus::MDS_HEADER << 16) |
                    ((mdsId & 0xF) << 16) |
                    ((numBytesInChunk >> 8) & 0xFF) << 8 |
                    (numBytesInChunk & 0xFF);

    uint32_t int2 = ((numChunks >> 8) & 0xFF) << 24 |
                    (numChunks & 0xFF) << 16 |
                    ((chunkIndex >> 8) & 0xFF) << 8 |
                    (chunkIndex & 0xFF);

    uint32_t int3 = ((manufacturerId >> 8) & 0xFF) << 24 |
                    (manufacturerId & 0xFF) << 16 |
                    ((deviceId >> 8) & 0xFF) << 8 |
                    (deviceId & 0xFF);

    uint32_t int4 = ((subId >> 8) & 0xFF) << 24 |
                    (subId & 0xFF) << 16 |
                    ((subId2 >> 8) & 0xFF) << 8 |
                    (subId2 & 0xFF);

    return Ump(int1, int2, int3, int4);
}

Ump UmpFactory::mdsGetPayloadOf(uint8_t group, uint8_t mdsId, const std::vector<uint8_t>& srcData,
                                 int offset, int numBytes) {
    constexpr int MDS_PAYLOAD_SIZE = 14;
    int size = std::min(numBytes - offset, MDS_PAYLOAD_SIZE);
    if (offset + size > static_cast<int>(srcData.size())) {
        size = srcData.size() - offset;
    }

    uint32_t int1 = (static_cast<uint32_t>(MessageType::SYSEX8_MDS) << 28) |
                    ((group & 0xF) << 24) |
                    ((Midi2BinaryChunkStatus::MDS_PAYLOAD | (mdsId & 0xF)) << 16);

    uint32_t int2 = 0;
    uint32_t int3 = 0;
    uint32_t int4 = 0;

    for (int i = 0; i < size && i < 14; i++) {
        uint8_t byte = (offset + i < static_cast<int>(srcData.size())) ? srcData[offset + i] : 0;
        if (i < 6) {
            if (i < 2) {
                int1 |= static_cast<uint32_t>(byte) << ((1 - i) * 8);
            } else {
                int2 |= static_cast<uint32_t>(byte) << ((5 - i) * 8);
            }
        } else if (i < 10) {
            int3 |= static_cast<uint32_t>(byte) << ((9 - i) * 8);
        } else {
            int4 |= static_cast<uint32_t>(byte) << ((13 - i) * 8);
        }
    }

    return Ump(int1, int2, int3, int4);
}

void UmpFactory::mdsProcess(uint8_t group, uint8_t mdsId, const std::vector<uint8_t>& data,
                            std::function<void(const Ump&, int, int)> callback) {
    int numChunks = mdsGetChunkCount(data.size());
    constexpr int MAX_CHUNK_SIZE = 14 * 65535;

    for (int c = 0; c < numChunks; c++) {
        int chunkSize = (c + 1 == numChunks) ? (data.size() % MAX_CHUNK_SIZE) : MAX_CHUNK_SIZE;
        if (chunkSize == 0) chunkSize = MAX_CHUNK_SIZE;
        int numPayloads = mdsGetPayloadCount(chunkSize);

        for (int p = 0; p < numPayloads; p++) {
            int offset = 14 * (65536 * c + p);
            if (offset < static_cast<int>(data.size())) {
                callback(mdsGetPayloadOf(group, mdsId, data, offset, chunkSize), c, p);
            }
        }
    }
}

std::vector<Ump> UmpFactory::mds(uint8_t group, const std::vector<uint8_t>& data, uint8_t mdsId) {
    std::vector<Ump> result;
    mdsProcess(group, mdsId, data, [&result](const Ump& ump, int, int) {
        result.push_back(ump);
    });
    return result;
}

Ump UmpFactory::sysexGetPacketOf(MessageType message_type, uint8_t group,
                                    const std::vector<uint8_t>& src_data, int packet_index, int radix, bool hasStreamId, uint8_t streamId) {
    // For sysex7, strip F0/F7 markers. For sysex8, use full data as-is.
    int sysex_length;
    int data_start;
    if (message_type == MessageType::SYSEX7) {
        sysex_length = sysex7GetSysexLength(src_data);
        data_start = src_data.size() > 0 && src_data[0] == 0xF0 ? 1 : 0;
    } else {
        sysex_length = src_data.size();
        data_start = 0;
    }

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

    int data_pos = data_start + packet_index * radix;

    int remaining_bytes = sysex_length - packet_index * radix;
    int packet_bytes = std::min(remaining_bytes, radix);
    int packet_bytes_field = packet_bytes;
    if (hasStreamId && message_type == MessageType::SYSEX8_MDS) {
        packet_bytes_field += 1;
    }

    uint32_t int1 = (static_cast<uint32_t>(message_type) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    ((static_cast<uint32_t>(status) | (packet_bytes_field & 0xF)) << 16);

    if (hasStreamId) {
        int1 |= static_cast<uint32_t>(streamId) << 8;
        if (packet_bytes > 0)
            int1 |= src_data[data_pos];
    } else {
        if (packet_bytes > 0)
            int1 |= src_data[data_pos] << 8;
        if (packet_bytes > 1)
            int1 |= src_data[data_pos + 1];
    }

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

void UmpFactory::umpStreamTextProcess(uint8_t status, const std::vector<uint8_t>& text,
                                       std::function<void(const Ump&)> callback,
                                       int capacity, uint8_t dataPrefix, bool hasDataPrefix) {
    auto textBytesToUmp = [](const std::vector<uint8_t>& bytes, int offset) -> uint32_t {
        uint32_t result = 0;
        for (int i = 0; i < 4; i++) {
            if (offset + i < static_cast<int>(bytes.size())) {
                result |= static_cast<uint32_t>(bytes[offset + i]) << (24 - i * 8);
            }
        }
        return result;
    };

    auto createPacket = [&](uint8_t format, int index) -> Ump {
        uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                        (static_cast<uint32_t>(format & 0x3) << 26) |
                        (static_cast<uint32_t>(status) << 16);

        if (hasDataPrefix) {
            uint8_t firstByte = (index < static_cast<int>(text.size())) ? text[index] : 0;
            int1 |= (static_cast<uint32_t>(dataPrefix) << 8) | firstByte;
            return Ump(int1,
                      textBytesToUmp(text, index + 1),
                      textBytesToUmp(text, index + 5),
                      textBytesToUmp(text, index + 9));
        } else {
            uint8_t byte1 = (index < static_cast<int>(text.size())) ? text[index] : 0;
            uint8_t byte2 = (index + 1 < static_cast<int>(text.size())) ? text[index + 1] : 0;
            int1 |= (static_cast<uint32_t>(byte1) << 8) | byte2;
            return Ump(int1,
                      textBytesToUmp(text, index + 2),
                      textBytesToUmp(text, index + 6),
                      textBytesToUmp(text, index + 10));
        }
    };

    if (static_cast<int>(text.size()) <= capacity) {
        callback(createPacket(0, 0));
    } else {
        callback(createPacket(1, 0));
        int numPackets = (text.size() + capacity - 1) / capacity;
        for (int i = 1; i < numPackets - 1; i++) {
            callback(createPacket(2, i * capacity));
        }
        callback(createPacket(3, (numPackets - 1) * capacity));
    }
}

std::vector<Ump> UmpFactory::umpStreamText(uint8_t status, const std::vector<uint8_t>& text) {
    std::vector<Ump> result;
    umpStreamTextProcess(status, text, [&result](const Ump& ump) {
        result.push_back(ump);
    });
    return result;
}

Ump UmpFactory::endpointDiscovery(uint8_t umpVersionMajor, uint8_t umpVersionMinor, uint8_t filterBitmap) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::ENDPOINT_DISCOVERY) << 16) |
                    (static_cast<uint32_t>(umpVersionMajor) << 8) |
                    umpVersionMinor;
    return Ump(int1, filterBitmap & 0x1F, 0, 0);
}

Ump UmpFactory::endpointInfoNotification(uint8_t umpVersionMajor, uint8_t umpVersionMinor,
                                          bool isStaticFunctionBlock, uint8_t functionBlockCount,
                                          bool midi2Capable, bool midi1Capable,
                                          bool supportsRxJR, bool supportsTxJR) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::ENDPOINT_INFO) << 16) |
                    (static_cast<uint32_t>(umpVersionMajor) << 8) |
                    umpVersionMinor;
    uint32_t int2 = (static_cast<uint32_t>(functionBlockCount) << 24) |
                    (isStaticFunctionBlock ? 0x80000000 : 0) |
                    (midi2Capable ? 0x200 : 0) |
                    (midi1Capable ? 0x100 : 0) |
                    (supportsRxJR ? 2 : 0) |
                    (supportsTxJR ? 1 : 0);
    return Ump(int1, int2, 0, 0);
}

Ump UmpFactory::deviceIdentityNotification(uint32_t manufacturer, uint16_t family, uint16_t modelNumber, uint32_t softwareRevisionLevel) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::DEVICE_IDENTITY) << 16);
    uint32_t int3 = (static_cast<uint32_t>(family) << 16) | modelNumber;
    return Ump(int1, manufacturer, int3, softwareRevisionLevel);
}

std::vector<Ump> UmpFactory::endpointNameNotification(const std::string& name) {
    return endpointNameNotification(std::vector<uint8_t>(name.begin(), name.end()));
}

std::vector<Ump> UmpFactory::endpointNameNotification(const std::vector<uint8_t>& name) {
    return umpStreamText(UmpStreamStatus::ENDPOINT_NAME, name);
}

std::vector<Ump> UmpFactory::productInstanceIdNotification(const std::string& id) {
    return productInstanceIdNotification(std::vector<uint8_t>(id.begin(), id.end()));
}

std::vector<Ump> UmpFactory::productInstanceIdNotification(const std::vector<uint8_t>& id) {
    return umpStreamText(UmpStreamStatus::PRODUCT_INSTANCE_ID, id);
}

Ump UmpFactory::streamConfigRequest(uint8_t protocol, bool rxJRTimestamp, bool txJRTimestamp) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::STREAM_CONFIG_REQUEST) << 16) |
                    (static_cast<uint32_t>(protocol) << 8) |
                    (rxJRTimestamp ? 2 : 0) |
                    (txJRTimestamp ? 1 : 0);
    return Ump(int1, 0, 0, 0);
}

Ump UmpFactory::streamConfigNotification(uint8_t protocol, bool rxJRTimestamp, bool txJRTimestamp) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::STREAM_CONFIG_NOTIFICATION) << 16) |
                    (static_cast<uint32_t>(protocol) << 8) |
                    (rxJRTimestamp ? 2 : 0) |
                    (txJRTimestamp ? 1 : 0);
    return Ump(int1, 0, 0, 0);
}

Ump UmpFactory::functionBlockDiscovery(uint8_t fbNumber, uint8_t filter) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::FUNCTION_BLOCK_DISCOVERY) << 16) |
                    (static_cast<uint32_t>(fbNumber) << 8) |
                    filter;
    return Ump(int1, 0, 0, 0);
}

Ump UmpFactory::functionBlockInfoNotification(bool isFbActive, uint8_t fbNumber, uint8_t uiHint, uint8_t midi1, uint8_t direction,
                                               uint8_t firstGroup, uint8_t numberOfGroupsSpanned,
                                               uint8_t midiCIMessageVersionFormat, uint8_t maxSysEx8Streams) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::UMP_STREAM) << 28) |
                    (static_cast<uint32_t>(UmpStreamStatus::FUNCTION_BLOCK_INFO) << 16) |
                    (isFbActive ? 0x8000 : 0) |
                    (static_cast<uint32_t>(fbNumber) << 8) |
                    ((uiHint & 0x3) << 4) |
                    ((midi1 & 0x3) << 2) |
                    (direction & 0x3);
    uint32_t int2 = (static_cast<uint32_t>(firstGroup) << 24) |
                    (static_cast<uint32_t>(numberOfGroupsSpanned) << 16) |
                    (static_cast<uint32_t>(midiCIMessageVersionFormat) << 8) |
                    (maxSysEx8Streams & 0xFF);
    return Ump(int1, int2, 0, 0);
}

std::vector<Ump> UmpFactory::functionBlockNameNotification(uint8_t blockNumber, const std::string& name) {
    std::vector<uint8_t> nameBytes(name.begin(), name.end());
    std::vector<Ump> result;
    umpStreamTextProcess(UmpStreamStatus::FUNCTION_BLOCK_NAME, nameBytes,
                        [&result](const Ump& ump) { result.push_back(ump); },
                        13, blockNumber, true);
    return result;
}

Ump UmpFactory::startOfClip() {
    return Ump(0xF0200000, 0, 0, 0);
}

Ump UmpFactory::endOfClip() {
    return Ump(0xF0210000, 0, 0, 0);
}

void UmpFactory::flexDataProcess(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status,
                                  const std::vector<uint8_t>& text, std::function<void(const Ump&)> callback) {
    auto textBytesToUmp = [](const std::vector<uint8_t>& bytes, int offset) -> uint32_t {
        uint32_t result = 0;
        for (int i = 0; i < 4; i++) {
            if (offset + i < static_cast<int>(bytes.size())) {
                result |= static_cast<uint32_t>(bytes[offset + i]) << (24 - i * 8);
            }
        }
        return result;
    };

    auto createPacket = [&](uint8_t format, int index) -> Ump {
        uint32_t int1 = (static_cast<uint32_t>(MessageType::FLEX_DATA) << 28) |
                        (static_cast<uint32_t>(group & 0xF) << 24) |
                        (static_cast<uint32_t>(format & 0x3) << 22) |
                        (static_cast<uint32_t>(address & 0xF) << 20) |
                        (static_cast<uint32_t>(channel & 0xF) << 16) |
                        (static_cast<uint32_t>(statusBank) << 8) |
                        status;
        return Ump(int1,
                  textBytesToUmp(text, index),
                  textBytesToUmp(text, index + 4),
                  textBytesToUmp(text, index + 8));
    };

    constexpr int capacity = 12;
    if (static_cast<int>(text.size()) < 13) {
        callback(createPacket(0, 0));
    } else {
        callback(createPacket(1, 0));
        int numPackets = (text.size() + capacity - 1) / capacity;
        for (int i = 1; i < numPackets - 1; i++) {
            callback(createPacket(2, i * capacity));
        }
        callback(createPacket(3, (numPackets - 1) * capacity));
    }
}

std::vector<Ump> UmpFactory::flexDataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status, const std::string& text) {
    return flexDataText(group, address, channel, statusBank, status, std::vector<uint8_t>(text.begin(), text.end()));
}

std::vector<Ump> UmpFactory::flexDataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status, const std::vector<uint8_t>& text) {
    std::vector<Ump> result;
    flexDataProcess(group, address, channel, statusBank, status, text, [&result](const Ump& ump) {
        result.push_back(ump);
    });
    return result;
}

Ump UmpFactory::flexDataCompleteBinary(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusByte, uint32_t int2, uint32_t int3, uint32_t int4) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::FLEX_DATA) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    (static_cast<uint32_t>(address & 0xF) << 20) |
                    (static_cast<uint32_t>(channel & 0xF) << 16) |
                    statusByte;
    return Ump(int1, int2, int3, int4);
}

Ump UmpFactory::tempo(uint8_t group, uint8_t channel, uint32_t numberOf10NanosecondsPerQuarterNote) {
    return flexDataCompleteBinary(group, 1, channel, FlexDataStatus::TEMPO, numberOf10NanosecondsPerQuarterNote);
}

Ump UmpFactory::timeSignatureDirect(uint8_t group, uint8_t channel, uint8_t numerator, uint8_t rawDenominator, uint8_t numberOf32Notes) {
    uint32_t int2 = (static_cast<uint32_t>(numerator) << 24) |
                    (static_cast<uint32_t>(rawDenominator) << 16) |
                    (static_cast<uint32_t>(numberOf32Notes) << 8);
    return flexDataCompleteBinary(group, 1, channel, FlexDataStatus::TIME_SIGNATURE, int2);
}

Ump UmpFactory::metronome(uint8_t group, uint8_t channel, uint8_t numClocksPerPrimaryClick, uint8_t barAccent1, uint8_t barAccent2, uint8_t barAccent3, uint8_t numSubdivisionClick1, uint8_t numSubdivisionClick2) {
    uint32_t int2 = (static_cast<uint32_t>(numClocksPerPrimaryClick) << 24) |
                    (static_cast<uint32_t>(barAccent1) << 16) |
                    (static_cast<uint32_t>(barAccent2) << 8) |
                    barAccent3;
    uint32_t int3 = (static_cast<uint32_t>(numSubdivisionClick1) << 24) |
                    (static_cast<uint32_t>(numSubdivisionClick2) << 16);
    return flexDataCompleteBinary(group, 1, channel, FlexDataStatus::METRONOME, int2, int3);
}

Ump UmpFactory::keySignature(uint8_t group, uint8_t address, uint8_t channel, int8_t sharpsOrFlats, uint8_t tonicNote) {
    uint8_t sharpsOrFlatsValue = (sharpsOrFlats < 0) ? (sharpsOrFlats + 0x10) : sharpsOrFlats;
    uint32_t int2 = (static_cast<uint32_t>(sharpsOrFlatsValue) << 28) |
                    (static_cast<uint32_t>(tonicNote) << 24);
    return flexDataCompleteBinary(group, address, channel, FlexDataStatus::KEY_SIGNATURE, int2);
}

Ump UmpFactory::chordName(uint8_t group, uint8_t address, uint8_t channel,
                          int8_t tonicSharpsFlats, uint8_t chordTonic, uint8_t chordType,
                          uint8_t alter1, uint8_t alter2, uint8_t alter3, uint8_t alter4,
                          int8_t bassSharpsFlats, uint8_t bassNote, uint8_t bassChordType,
                          uint8_t bassAlter1, uint8_t bassAlter2) {
    uint8_t tonicSharpsFlatsValue = (tonicSharpsFlats < 0) ? (tonicSharpsFlats + 0x10) : tonicSharpsFlats;
    uint8_t bassSharpsFlatsValue = (bassSharpsFlats < 0) ? (bassSharpsFlats + 0x10) : bassSharpsFlats;

    uint32_t int2 = (static_cast<uint32_t>(tonicSharpsFlatsValue) << 28) |
                    (static_cast<uint32_t>(chordTonic) << 24) |
                    (static_cast<uint32_t>(chordType) << 16) |
                    (static_cast<uint32_t>(alter1) << 8) |
                    alter2;
    uint32_t int3 = (static_cast<uint32_t>(alter3) << 24) |
                    (static_cast<uint32_t>(alter4) << 16);
    uint32_t int4 = (static_cast<uint32_t>(bassSharpsFlatsValue) << 28) |
                    (static_cast<uint32_t>(bassNote) << 24) |
                    (static_cast<uint32_t>(bassChordType) << 16) |
                    (static_cast<uint32_t>(bassAlter1) << 8) |
                    bassAlter2;
    return flexDataCompleteBinary(group, address, channel, FlexDataStatus::CHORD_NAME, int2, int3, int4);
}

std::vector<Ump> UmpFactory::metadataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::string& text) {
    return flexDataText(group, address, channel, 1, status, text);
}

std::vector<Ump> UmpFactory::metadataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::vector<uint8_t>& text) {
    return flexDataText(group, address, channel, 1, status, text);
}

std::vector<Ump> UmpFactory::performanceText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::string& text) {
    return flexDataText(group, address, channel, 2, status, text);
}

std::vector<Ump> UmpFactory::performanceText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::vector<uint8_t>& text) {
    return flexDataText(group, address, channel, 2, status, text);
}

} // namespace umppi
