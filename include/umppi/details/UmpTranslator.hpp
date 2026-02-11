#pragma once

#include <umppi/details/Ump.hpp>
#include <umppi/details/Common.hpp>
#include <vector>
#include <cstdint>
#include <functional>

namespace umppi {

namespace UmpTranslationResult {
    constexpr int OK = 0;
    constexpr int INVALID_SYSEX = 0x10;
    constexpr int INVALID_DTE_SEQUENCE = 0x11;
    constexpr int INVALID_STATUS = 0x13;
    constexpr int INCOMPLETE_SYSEX7 = 0x20;
}

struct Midi1ToUmpTranslatorContext {
    std::vector<uint8_t> midi1;
    bool allowReorderedDTE;
    int midiProtocol;
    int group;
    bool useSysex8;
    bool isMidi1Smf;

    size_t midi1Pos;
    std::vector<Ump> output;

    uint16_t rpnState;
    uint16_t nrpnState;
    uint16_t dteState;
    uint16_t bankState;

    int tempo;

    Midi1ToUmpTranslatorContext(const std::vector<uint8_t>& midi1Data,
                                int groupNum,
                                bool allowReorderedDTE = false,
                                int midiProtocol = static_cast<int>(MidiTransportProtocol::UMP),
                                bool useSysex8 = false,
                                bool isMidi1Smf = false)
        : midi1(midi1Data), allowReorderedDTE(allowReorderedDTE),
          midiProtocol(midiProtocol), group(groupNum), useSysex8(useSysex8),
          isMidi1Smf(isMidi1Smf), midi1Pos(0), rpnState(0x8080),
          nrpnState(0x8080), dteState(0x8080), bankState(0x8080), tempo(500000) {}
};

struct UmpToMidi1BytesTranslatorContext {
    int deltaTimeMasterClock;
    bool treatJRTimestampAsSmfDeltaTime;
    bool skipDeltaTime;

    UmpToMidi1BytesTranslatorContext(int deltaTimeMasterClock = 192,
                                     bool treatJRTimestampAsSmfDeltaTime = false,
                                     bool skipDeltaTime = false)
        : deltaTimeMasterClock(deltaTimeMasterClock),
          treatJRTimestampAsSmfDeltaTime(treatJRTimestampAsSmfDeltaTime),
          skipDeltaTime(skipDeltaTime) {}
};

class UmpTranslator {
public:
    static int translateUmpToMidi1Bytes(std::vector<uint8_t>& dst,
                                        const std::vector<Ump>& src,
                                        const UmpToMidi1BytesTranslatorContext& context = UmpToMidi1BytesTranslatorContext());

    static int translateSingleUmpToMidi1Bytes(std::vector<uint8_t>& dst,
                                              const Ump& ump,
                                              size_t dstOffset = 0,
                                              int deltaTime = -1,
                                              std::vector<uint8_t>* sysex = nullptr);

    static int translateMidi1BytesToUmp(Midi1ToUmpTranslatorContext& context);

    static void translateMidi1UmpToMidi2Ump(std::vector<Ump>& dst, const std::vector<Ump>& src);

    static void translateMidi2UmpToMidi1Ump(std::vector<Ump>& dst, const std::vector<Ump>& src);

private:
    static uint64_t convertMidi1DteToUmp(Midi1ToUmpTranslatorContext& context, int channel);
    static int getMidi1MessageSize(uint8_t statusByte);
};

} // namespace umppi
