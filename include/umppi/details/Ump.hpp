#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <string>

namespace umppi {

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

    uint8_t getMessageType() const { return static_cast<uint8_t>((int1 >> 28) & 0xF); }
    uint8_t getGroup() const { return static_cast<uint8_t>((int1 >> 24) & 0xF); }
    uint8_t getStatusByte() const { return static_cast<uint8_t>((int1 >> 16) & 0xFF); }
    uint8_t getStatusCode() const { return getStatusByte() & 0xF0; }
    uint8_t getChannelInGroup() const { return getStatusByte() & 0xF; }
    uint8_t getGroupAndChannel() const { return (getGroup() << 4) | getChannelInGroup(); }

    int getSizeInInts() const;
    int getSizeInBytes() const { return getSizeInInts() * 4; }

    uint8_t getMidi1Msb() const { return static_cast<uint8_t>((int1 >> 8) & 0xFF); }
    uint8_t getMidi1Lsb() const { return static_cast<uint8_t>(int1 & 0xFF); }
    uint8_t getMidi1Note() const { return getMidi1Msb(); }
    uint8_t getMidi1Velocity() const { return getMidi1Lsb(); }
    uint8_t getMidi1CCIndex() const { return getMidi1Msb(); }
    uint8_t getMidi1CCData() const { return getMidi1Lsb(); }
    uint8_t getMidi1Program() const { return getMidi1Msb(); }
    uint16_t getMidi1PitchBendData() const { return getMidi1Msb() + (getMidi1Lsb() << 7); }

    bool isJRClock() const;
    uint16_t getJRClock() const;
    bool isJRTimestamp() const;
    uint16_t getJRTimestamp() const;
    bool isDCTPQ() const;
    uint16_t getDCTPQ() const;
    bool isDeltaClockstamp() const;
    uint32_t getDeltaClockstamp() const;

    bool isTempo() const;
    uint32_t getTempo() const { return int2; }

    bool isTimeSignature() const;
    uint8_t getTimeSignatureNumerator() const { return static_cast<uint8_t>((int2 >> 24) & 0xFF); }
    uint8_t getTimeSignatureDenominator() const { return static_cast<uint8_t>((int2 >> 16) & 0xFF); }

    void toBytes(std::vector<uint8_t>& bytes, size_t offset = 0) const;
    std::vector<uint8_t> toBytes() const;
    std::array<uint32_t, 4> toInts() const { return {int1, int2, int3, int4}; }

    static std::vector<Ump> fromBytes(const uint8_t* bytes, size_t count);
    static std::vector<Ump> fromBytes(const std::vector<uint8_t>& bytes) {
        return fromBytes(bytes.data(), bytes.size());
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

}
