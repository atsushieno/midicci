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
    static std::vector<uint32_t> jrTimestamps(uint64_t senderClockTimestampTicks);
    static std::vector<uint32_t> jrTimestamps(double senderClockTimestampSeconds);
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
    static uint32_t midi1PitchBendSplit(uint8_t group, uint8_t channel, uint8_t dataLSB, uint8_t dataMSB);

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
    static uint64_t midi2RelativeRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32);
    static uint64_t midi2RelativeNRPN(uint8_t group, uint8_t channel, uint8_t msb, uint8_t lsb, uint32_t data32);
    static uint64_t midi2PerNoteRCC(uint8_t group, uint8_t channel, uint8_t note, uint8_t index, uint32_t data32);
    static uint64_t midi2PerNoteACC(uint8_t group, uint8_t channel, uint8_t note, uint8_t index, uint32_t data32);
    static uint64_t midi2PerNoteManagement(uint8_t group, uint8_t channel, uint8_t note, uint8_t optionFlags);
    static uint64_t midi2PerNotePitchBend(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32);
    static uint64_t midi2PerNotePitchBendDirect(uint8_t group, uint8_t channel, uint8_t note, uint32_t data32);

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

    // SysEx 8-bit Messages
    static int sysex8GetPacketCount(int numBytes);
    static Ump sysex8GetPacketOf(uint8_t group, uint8_t streamId, const std::vector<uint8_t>& src_data, int packet_index);
    static void sysex8Process(uint8_t group, const std::vector<uint8_t>& src_data, uint8_t streamId,
                               std::function<void(const Ump&)> callback);
    static std::vector<Ump> sysex8(uint8_t group, const std::vector<uint8_t>& src_data, uint8_t streamId = 0);

    // Mixed Data Set (MDS) Messages
    static int mdsGetChunkCount(int numTotalBytesInMDS);
    static int mdsGetPayloadCount(int numTotalBytesInChunk);
    static Ump mdsGetHeader(uint8_t group, uint8_t mdsId, uint16_t numBytesInChunk, uint16_t numChunks,
                            uint16_t chunkIndex, uint16_t manufacturerId, uint16_t deviceId,
                            uint16_t subId, uint16_t subId2);
    static Ump mdsGetPayloadOf(uint8_t group, uint8_t mdsId, const std::vector<uint8_t>& srcData,
                               int offset, int numBytes);
    static void mdsProcess(uint8_t group, uint8_t mdsId, const std::vector<uint8_t>& data,
                           std::function<void(const Ump&, int, int)> callback);
    static std::vector<Ump> mds(uint8_t group, const std::vector<uint8_t>& data, uint8_t mdsId = 0);

    // UMP Stream Messages
    static Ump endpointDiscovery(uint8_t umpVersionMajor, uint8_t umpVersionMinor, uint8_t filterBitmap);
    static Ump endpointInfoNotification(uint8_t umpVersionMajor, uint8_t umpVersionMinor,
                                        bool isStaticFunctionBlock, uint8_t functionBlockCount,
                                        bool midi2Capable, bool midi1Capable,
                                        bool supportsRxJR, bool supportsTxJR);
    static Ump deviceIdentityNotification(uint32_t manufacturer, uint16_t family, uint16_t modelNumber, uint32_t softwareRevisionLevel);
    static std::vector<Ump> endpointNameNotification(const std::string& name);
    static std::vector<Ump> endpointNameNotification(const std::vector<uint8_t>& name);
    static std::vector<Ump> productInstanceIdNotification(const std::string& id);
    static std::vector<Ump> productInstanceIdNotification(const std::vector<uint8_t>& id);
    static Ump streamConfigRequest(uint8_t protocol, bool rxJRTimestamp, bool txJRTimestamp);
    static Ump streamConfigNotification(uint8_t protocol, bool rxJRTimestamp, bool txJRTimestamp);
    static Ump functionBlockDiscovery(uint8_t fbNumber, uint8_t filter);
    static Ump functionBlockInfoNotification(bool isFbActive, uint8_t fbNumber, uint8_t uiHint, uint8_t midi1, uint8_t direction,
                                             uint8_t firstGroup, uint8_t numberOfGroupsSpanned,
                                             uint8_t midiCIMessageVersionFormat, uint8_t maxSysEx8Streams);
    static std::vector<Ump> functionBlockNameNotification(uint8_t blockNumber, const std::string& name);
    static Ump startOfClip();
    static Ump endOfClip();

    // Flex Data Messages
    static std::vector<Ump> flexDataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status, const std::string& text);
    static std::vector<Ump> flexDataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status, const std::vector<uint8_t>& text);
    static Ump flexDataCompleteBinary(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusByte, uint32_t int2, uint32_t int3 = 0, uint32_t int4 = 0);
    static Ump tempo(uint8_t group, uint8_t channel, uint32_t numberOf10NanosecondsPerQuarterNote);
    static Ump timeSignatureDirect(uint8_t group, uint8_t channel, uint8_t numerator, uint8_t rawDenominator, uint8_t numberOf32Notes);
    static Ump metronome(uint8_t group, uint8_t channel, uint8_t numClocksPerPrimaryClick, uint8_t barAccent1, uint8_t barAccent2, uint8_t barAccent3, uint8_t numSubdivisionClick1, uint8_t numSubdivisionClick2);
    static Ump keySignature(uint8_t group, uint8_t address, uint8_t channel, int8_t sharpsOrFlats, uint8_t tonicNote);
    static Ump chordName(uint8_t group, uint8_t address, uint8_t channel,
                         int8_t tonicSharpsFlats, uint8_t chordTonic, uint8_t chordType,
                         uint8_t alter1, uint8_t alter2, uint8_t alter3, uint8_t alter4,
                         int8_t bassSharpsFlats, uint8_t bassNote, uint8_t bassChordType,
                         uint8_t bassAlter1, uint8_t bassAlter2);
    static std::vector<Ump> metadataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::string& text);
    static std::vector<Ump> metadataText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::vector<uint8_t>& text);
    static std::vector<Ump> performanceText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::string& text);
    static std::vector<Ump> performanceText(uint8_t group, uint8_t address, uint8_t channel, uint8_t status, const std::vector<uint8_t>& text);

private:
    static Ump sysexGetPacketOf(MessageType message_type, uint8_t group,
                                   const std::vector<uint8_t>& src_data, int packet_index, int radix,
                                   bool hasStreamId, uint8_t streamId);
    static int getPacketCountCommon(int numBytes, int radix);

    static void umpStreamTextProcess(uint8_t status, const std::vector<uint8_t>& text,
                                      std::function<void(const Ump&)> callback,
                                      int capacity = 14, uint8_t dataPrefix = 0, bool hasDataPrefix = false);
    static std::vector<Ump> umpStreamText(uint8_t status, const std::vector<uint8_t>& text);

    static void flexDataProcess(uint8_t group, uint8_t address, uint8_t channel, uint8_t statusBank, uint8_t status,
                                const std::vector<uint8_t>& text, std::function<void(const Ump&)> callback);

    static constexpr int SYSEX7_RADIX = 6;
    static constexpr int SYSEX8_RADIX = 13;
    static constexpr int JR_TIMESTAMP_TICKS_PER_SECOND = 31250;
    static constexpr uint8_t MIDI_2_0_RESERVED = 0;
};

} // namespace umppi
