#pragma once

#include "midicci/details/ump/Ump.hpp"
#include <vector>
#include <cstdint>
#include <functional>

namespace midicci {
namespace ump {

// Translation result constants
namespace UmpTranslationResult {
    constexpr int OK = 0;
    constexpr int INVALID_SYSEX = 0x10;
    constexpr int INVALID_DTE_SEQUENCE = 0x11;
    constexpr int INVALID_STATUS = 0x13;
    constexpr int INCOMPLETE_SYSEX7 = 0x20;
}

// MIDI Transport Protocol constants
namespace MidiTransportProtocol {
    constexpr int MIDI1 = 1;
    constexpr int UMP = 2;
}

// Context for MIDI1 to UMP translation
struct Midi1ToUmpTranslatorContext {
    std::vector<uint8_t> midi1;
    bool allowReorderedDTE;
    int midiProtocol;
    int group;
    bool useSysex8;
    bool isMidi1Smf;
    
    // Current position in MIDI1 stream
    size_t midi1Pos;
    
    // Output UMP messages
    std::vector<Ump> output;
    
    // DTE conversion state - initialized to 0x8080 (invalid)
    uint16_t rpnState;
    uint16_t nrpnState;
    uint16_t dteState;
    
    // Bank Select state
    uint16_t bankState;
    
    // Tempo for SMF conversion
    int tempo;
    
    Midi1ToUmpTranslatorContext(const std::vector<uint8_t>& midi1Data, 
                                int groupNum,
                                bool allowReorderedDTE = false,
                                int midiProtocol = MidiTransportProtocol::UMP,
                                bool useSysex8 = false,
                                bool isMidi1Smf = false)
        : midi1(midi1Data), allowReorderedDTE(allowReorderedDTE), 
          midiProtocol(midiProtocol), group(groupNum), useSysex8(useSysex8), 
          isMidi1Smf(isMidi1Smf), midi1Pos(0), rpnState(0x8080), 
          nrpnState(0x8080), dteState(0x8080), bankState(0x8080), tempo(500000) {}
};

// Context for UMP to MIDI1 translation
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
    // Convert UMP stream to MIDI1 bytes
    static int translateUmpToMidi1Bytes(std::vector<uint8_t>& dst, 
                                        const std::vector<Ump>& src,
                                        const UmpToMidi1BytesTranslatorContext& context = UmpToMidi1BytesTranslatorContext());
    
    // Convert single UMP to MIDI1 bytes (main implementation function)
    static int translateSingleUmpToMidi1Bytes(std::vector<uint8_t>& dst, 
                                              const Ump& ump,
                                              size_t dstOffset = 0,
                                              int deltaTime = -1,
                                              std::vector<uint8_t>* sysex = nullptr);
    
    // Convert MIDI1 bytes to UMP
    static int translateMidi1BytesToUmp(Midi1ToUmpTranslatorContext& context);

private:
    // Helper function for MIDI1 DTE to UMP conversion
    static uint64_t convertMidi1DteToUmp(Midi1ToUmpTranslatorContext& context, int channel);
    
    // Helper to get MIDI1 message size
    static int getMidi1MessageSize(uint8_t statusByte);
};

} // namespace ump
} // namespace midicci