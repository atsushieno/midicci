#pragma once

#include <umppi/details/Ump.hpp>
#include <umppi/details/Common.hpp>
#include <vector>
#include <functional>

#undef JR_TIMESTAMP_TICKS_PER_SECOND
#undef MIDI_2_0_RESERVED

namespace umppi {

class UmpFactory {
public:
    // Utility Messages
    static uint32_t noop();
    static uint32_t jrClock(uint16_t senderClockTime16);
    static uint32_t jrClock(double senderClockTimeSeconds);
    static uint32_t jrTimestamp(uint16_t senderClockTimestamp16);
    static uint32_t jrTimestamp(double senderClockTimestampSeconds);
    static uint32_t dctpq(uint16_t numberOfTicksPerQuarterNote);
    static uint32_t deltaClockstamp(uint32_t ticks20);

    // System Messages
    static uint32_t systemMessage(uint8_t group, uint8_t status, uint8_t midi1Byte2, uint8_t midi1Byte3);

    // MIDI 1.0 Messages
    static uint32_t midi1Message(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4);
    static uint32_t midi1NoteOff(uint8_t group, uint8_t channel, uint8_t note, uint8_t velocity);
    static uint32_t midi1NoteOn(uint8_t group, uint8_t channel, uint8_t note, uint8_t velocity);
    static uint32_t midi1PAf(uint8_t group, uint8_t channel, uint8_t note, uint8_t data);
    static uint32_t midi1CC(uint8_t group, uint8_t channel, uint8_t index, uint8_t data);
    static uint32_t midi1Program(uint8_t group, uint8_t channel, uint8_t program);
    static uint32_t midi1CAf(uint8_t group, uint8_t channel, uint8_t data);
    static uint32_t midi1PitchBendDirect(uint8_t group, uint8_t channel, uint16_t data14);
    static uint32_t midi1PitchBend(uint8_t group, uint8_t channel, int16_t data);

    // MIDI 2.0 Messages
    static uint64_t midi2ChannelMessage8_8_16_16(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4, uint16_t short1, uint16_t short2);
    static uint64_t midi2ChannelMessage8_8_32(uint8_t group, uint8_t code, uint8_t channel, uint8_t byte3, uint8_t byte4, uint32_t rest32);

    static uint16_t pitch7_9(double pitch);
    static uint16_t pitch7_9Split(uint8_t semitone, double microtone0To1);

    static uint64_t midi2NoteOff(uint8_t group, uint8_t channel, uint8_t note, uint8_t attributeType8, uint16_t velocity16, uint16_t attributeData16);
    static uint64_t midi2NoteOn(uint8_t group, uint8_t channel, uint8_t note, uint8_t attributeType8, uint16_t velocity16, uint16_t attributeData16);
    static uint64_t midi2PAf(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32);
    static uint64_t midi2CC(uint8_t group, uint8_t channel, uint8_t index, uint32_t data32);
    static uint64_t midi2Program(uint8_t group, uint8_t channel, uint8_t options, uint8_t program, uint8_t bankMsb, uint8_t bankLsb);
    static uint64_t midi2CAf(uint8_t group, uint8_t channel, uint32_t data32);
    static uint64_t midi2PitchBendDirect(uint8_t group, uint8_t channel, uint32_t data32);
    static uint64_t midi2PitchBend(uint8_t group, uint8_t channel, int32_t data);
    static uint64_t midi2RPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32);
    static uint64_t midi2NRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32);

    // SysEx Messages
    static Ump sysex7Direct(uint8_t group, uint8_t status, uint8_t numBytes,
                              uint8_t data1 = 0, uint8_t data2 = 0, uint8_t data3 = 0,
                              uint8_t data4 = 0, uint8_t data5 = 0, uint8_t data6 = 0);

    static int sysex7GetSysexLength(const std::vector<uint8_t>& src_data);
    static int sysex7GetPacketCount(const std::vector<uint8_t>& src_data);
    static Ump sysex7GetPacketOf(uint8_t group, const std::vector<uint8_t>& src_data, int packet_index);
    static void sysex7Process(uint8_t group, const std::vector<uint8_t>& src_data,
                               std::function<void(const Ump&)> callback);
    static std::vector<Ump> sysex7(uint8_t group, const std::vector<uint8_t>& src_data);

    // UMP Stream Messages
    static Ump startOfClip();
    static Ump endOfClip();

private:
    static Ump sysexGetPacketOf(MessageType message_type, uint8_t group,
                                   const std::vector<uint8_t>& src_data, int packet_index, int radix,
                                   bool hasStreamId, uint8_t streamId);

    static constexpr int SYSEX7_RADIX = 6;
    static constexpr int JR_TIMESTAMP_TICKS_PER_SECOND = 31250;
    static constexpr uint8_t MIDI_2_0_RESERVED = 0;
};

} // namespace umppi
