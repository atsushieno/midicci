#include <umppi/details/Ump.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>
#include <sstream>
#include <iomanip>

namespace umppi {

int Ump::getSizeInInts() const {
    return umpSizeInInts(getMessageType());
}

bool Ump::isJRClock() const {
    return getMessageType() == MidiMessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::JR_CLOCK;
}

uint16_t Ump::getJRClock() const {
    return isJRClock() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isJRTimestamp() const {
    return getMessageType() == MidiMessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::JR_TIMESTAMP;
}

uint16_t Ump::getJRTimestamp() const {
    return isJRTimestamp() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isDCTPQ() const {
    return getMessageType() == MidiMessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::DCTPQ;
}

uint16_t Ump::getDCTPQ() const {
    return isDCTPQ() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isDeltaClockstamp() const {
    return getMessageType() == MidiMessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::DELTA_CLOCKSTAMP;
}

uint32_t Ump::getDeltaClockstamp() const {
    return isDeltaClockstamp() ? (int1 & 0xFFFFF) : 0;
}

bool Ump::isTempo() const {
    return getMessageType() == MidiMessageType::FLEX_DATA &&
           static_cast<uint8_t>(int1 & 0xFF) == FlexDataStatus::TEMPO;
}

bool Ump::isTimeSignature() const {
    return getMessageType() == MidiMessageType::FLEX_DATA &&
           static_cast<uint8_t>(int1 & 0xFF) == FlexDataStatus::TIME_SIGNATURE;
}

void Ump::toBytes(std::vector<uint8_t>& bytes, size_t offset) const {
    auto writeInt = [&bytes, offset](uint32_t value, size_t off) {
        bytes[offset + off] = static_cast<uint8_t>((value >> 24) & 0xFF);
        bytes[offset + off + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
        bytes[offset + off + 2] = static_cast<uint8_t>((value >> 8) & 0xFF);
        bytes[offset + off + 3] = static_cast<uint8_t>(value & 0xFF);
    };

    int size = getSizeInInts();
    if (bytes.size() < offset + size * 4) {
        bytes.resize(offset + size * 4);
    }

    writeInt(int1, 0);
    if (size >= 2) {
        writeInt(int2, 4);
    }
    if (size == 4) {
        writeInt(int3, 8);
        writeInt(int4, 12);
    }
}

std::vector<uint8_t> Ump::toBytes() const {
    std::vector<uint8_t> bytes;
    toBytes(bytes, 0);
    return bytes;
}

std::vector<Ump> Ump::fromBytes(const uint8_t* bytes, size_t count) {
    std::vector<Ump> result;
    size_t offset = 0;

    auto readInt = [bytes](size_t off) -> uint32_t {
        return (static_cast<uint32_t>(bytes[off]) << 24) |
               (static_cast<uint32_t>(bytes[off + 1]) << 16) |
               (static_cast<uint32_t>(bytes[off + 2]) << 8) |
               static_cast<uint32_t>(bytes[off + 3]);
    };

    while (offset < count) {
        uint32_t i1 = readInt(offset);
        uint8_t messageType = static_cast<uint8_t>((i1 >> 28) & 0xF);
        int sizeInInts = umpSizeInInts(messageType);

        if (offset + sizeInInts * 4 > count) {
            break;
        }

        if (sizeInInts == 1) {
            result.emplace_back(i1);
        } else if (sizeInInts == 2) {
            uint32_t i2 = readInt(offset + 4);
            result.emplace_back(i1, i2);
        } else if (sizeInInts == 4) {
            uint32_t i2 = readInt(offset + 4);
            uint32_t i3 = readInt(offset + 8);
            uint32_t i4 = readInt(offset + 12);
            result.emplace_back(i1, i2, i3, i4);
        }

        offset += sizeInInts * 4;
    }

    return result;
}

std::string Ump::toString() const {
    std::ostringstream oss;
    int size = getSizeInInts();

    oss << "[" << std::hex << std::setw(8) << std::setfill('0') << int1;
    if (size >= 2) {
        oss << ":" << std::setw(8) << std::setfill('0') << int2;
    }
    if (size == 4) {
        oss << ":" << std::setw(8) << std::setfill('0') << int3
            << ":" << std::setw(8) << std::setfill('0') << int4;
    }
    oss << "]";

    return oss.str();
}

}
