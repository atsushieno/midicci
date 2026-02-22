#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <span>

namespace umppi {

using UmpWordSpan = std::span<const uint32_t>;

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

class Ump {
public:
    uint32_t int1;
    uint32_t int2;
    uint32_t int3;
    uint32_t int4;

    Ump() : int1(0), int2(0), int3(0), int4(0) {}
    Ump(uint32_t i1) : int1(i1), int2(0), int3(0), int4(0) {}
    Ump(uint32_t i1, uint32_t i2) : int1(i1), int2(i2), int3(0), int4(0) {}
    Ump(uint32_t i1, uint32_t i2, uint32_t i3, uint32_t i4)
        : int1(i1), int2(i2), int3(i3), int4(i4) {}
    explicit Ump(uint64_t value) : int1(static_cast<uint32_t>(value >> 32)), int2(static_cast<uint32_t>(value & 0xFFFFFFFF)), int3(0), int4(0) {}

    // Basic properties
    MessageType getMessageType() const { return static_cast<MessageType>((int1 >> 28) & 0xF); }
    uint8_t getGroup() const { return static_cast<uint8_t>((int1 >> 24) & 0xF); }
    uint8_t getStatusByte() const { return static_cast<uint8_t>((int1 >> 16) & 0xFF); }
    uint8_t getStatusCode() const { return getStatusByte() & 0xF0; }
    uint8_t getChannelInGroup() const { return getStatusByte() & 0xF; }
    uint8_t getGroupAndChannel() const { return (getGroup() << 4) | getChannelInGroup(); }
    BinaryChunkStatus getBinaryChunkStatus() const;
    uint8_t getSysex7Size() const { return (int1 >> 16) & 0xF; }
    uint8_t getSysex8Size() const { return (int1 >> 16) & 0xF; }

    int getSizeInInts() const;
    int getSizeInBytes() const;

    // MIDI1 accessors
    uint8_t getMidi1Msb() const { return static_cast<uint8_t>((int1 >> 8) & 0x7F); }
    uint8_t getMidi1Lsb() const { return static_cast<uint8_t>(int1 & 0x7F); }
    uint8_t getMidi1Note() const { return getMidi1Msb(); }
    uint8_t getMidi1Velocity() const { return getMidi1Lsb(); }
    uint8_t getMidi1CCIndex() const { return getMidi1Msb(); }
    uint8_t getMidi1CCData() const { return getMidi1Lsb(); }
    uint8_t getMidi1Program() const { return getMidi1Msb(); }
    uint16_t getMidi1PitchBendData() const { return getMidi1Msb() + (getMidi1Lsb() << 7); }

    // MIDI2 accessors
    uint8_t getMidi2Note() const { return static_cast<uint8_t>((int1 >> 8) & 0x7F); }
    uint16_t getMidi2Velocity16() const { return static_cast<uint16_t>((int2 >> 16) & 0xFFFF); }
    uint32_t getMidi2PafData() const { return int2; }
    uint8_t getMidi2CcIndex() const { return static_cast<uint8_t>((int1 >> 8) & 0x7F); }
    uint32_t getMidi2CcData() const { return int2; }
    uint8_t getMidi2ProgramOptions() const { return static_cast<uint8_t>(int1 & 0x1); }
    uint8_t getMidi2ProgramProgram() const { return static_cast<uint8_t>((int2 >> 24) & 0x7F); }
    uint8_t getMidi2ProgramBankMsb() const { return static_cast<uint8_t>((int2 >> 8) & 0x7F); }
    uint8_t getMidi2ProgramBankLsb() const { return static_cast<uint8_t>(int2 & 0x7F); }
    uint32_t getMidi2CafData() const { return int2; }
    uint32_t getMidi2PitchBendData() const { return int2; }
    uint8_t getMidi2RpnMsb() const { return static_cast<uint8_t>((int1 >> 8) & 0x7F); }
    uint8_t getMidi2RpnLsb() const { return static_cast<uint8_t>(int1 & 0x7F); }
    uint32_t getMidi2RpnData() const { return int2; }
    uint8_t getMidi2NrpnMsb() const { return static_cast<uint8_t>((int1 >> 8) & 0x7F); }
    uint8_t getMidi2NrpnLsb() const { return static_cast<uint8_t>(int1 & 0x7F); }
    uint32_t getMidi2NrpnData() const { return int2; }

    // Timing accessors
    bool isJRClock() const;
    uint16_t getJRClock() const;
    bool isJRTimestamp() const;
    uint16_t getJRTimestamp() const;
    bool isDCTPQ() const;
    uint16_t getDCTPQ() const;
    bool isDeltaClockstamp() const;
    uint32_t getDeltaClockstamp() const;
    bool isStartOfClip() const;
    bool isEndOfClip() const;

    // Flex data accessors
    bool isTempo() const;
    uint32_t getTempo() const { return int2; }
    bool isTimeSignature() const;
    uint8_t getTimeSignatureNumerator() const { return static_cast<uint8_t>((int2 >> 24) & 0xFF); }
    uint8_t getTimeSignatureDenominator() const { return static_cast<uint8_t>((int2 >> 16) & 0xFF); }

    // Data conversion
    void toBytes(std::vector<uint8_t>& bytes, size_t offset = 0) const;
    std::vector<uint8_t> toBytes() const;
    std::vector<uint8_t> toPlatformBytes() const { return toBytes(); }
    std::array<uint32_t, 4> toInts() const { return {int1, int2, int3, int4}; }
    void toWords(std::vector<uint32_t>& words, size_t offset = 0) const;
    std::vector<uint32_t> toWords() const;

    static std::vector<Ump> fromBytes(const uint8_t* bytes, size_t count);
    static std::vector<Ump> fromBytes(const std::vector<uint8_t>& bytes) {
        return fromBytes(bytes.data(), bytes.size());
    }
    static std::vector<Ump> fromWords(const uint32_t* words, size_t count);
    static std::vector<Ump> fromWords(UmpWordSpan words) {
        return fromWords(words.data(), words.size());
    }

    std::string toString() const;

    bool operator==(const Ump& other) const {
        return int1 == other.int1 && int2 == other.int2 && int3 == other.int3 && int4 == other.int4;
    }
    bool operator!=(const Ump& other) const { return !(*this == other); }
};

inline int umpSizeInInts(uint8_t messageType) {
    if (messageType >= 5) return 4;
    if (messageType >= 3) return 2;
    return 1;
}

std::vector<Ump> parseUmpsFromBytes(const uint8_t* data, size_t start, size_t length);
std::vector<Ump> parseUmpsFromWords(const uint32_t* words, size_t count);
std::vector<Ump> parseUmpsFromWords(UmpWordSpan words);

}
