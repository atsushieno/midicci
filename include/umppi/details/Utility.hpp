#pragma once

#include <cstdint>
#include <utility>

namespace umppi {

struct Dc {
    int value;

    constexpr Dc(int v = 0) : value(v) {}

    constexpr bool operator==(const Dc& other) const { return value == other.value; }
    constexpr bool operator!=(const Dc& other) const { return value != other.value; }
    constexpr bool operator<(const Dc& other) const { return value < other.value; }
    constexpr bool operator<=(const Dc& other) const { return value <= other.value; }
    constexpr bool operator>(const Dc& other) const { return value > other.value; }
    constexpr bool operator>=(const Dc& other) const { return value >= other.value; }
};

template<typename T>
struct Timed {
    Dc duration;
    T value;

    Timed(Dc d, T v) : duration(d), value(std::move(v)) {}
};

inline constexpr uint8_t toUnsigned(int8_t value) {
    return static_cast<uint8_t>(value);
}

inline constexpr uint16_t toUnsigned(int16_t value) {
    return static_cast<uint16_t>(value);
}

inline constexpr uint32_t toUnsigned(int32_t value) {
    return static_cast<uint32_t>(value);
}

inline uint16_t readBe16(const uint8_t* data) {
    return static_cast<uint16_t>((data[0] << 8) | data[1]);
}

inline uint32_t readBe32(const uint8_t* data) {
    return (static_cast<uint32_t>(data[0]) << 24) |
           (static_cast<uint32_t>(data[1]) << 16) |
           (static_cast<uint32_t>(data[2]) << 8) |
            static_cast<uint32_t>(data[3]);
}

inline void writeBe16(uint8_t* data, uint16_t value) {
    data[0] = static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value & 0xFF);
}

inline void writeBe32(uint8_t* data, uint32_t value) {
    data[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    data[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    data[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[3] = static_cast<uint8_t>(value & 0xFF);
}

} // namespace umppi
