#include <umppi/details/Ump.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>
#include <sstream>
#include <iomanip>

namespace umppi {

int Ump::getSizeInInts() const {
    return umpSizeInInts(static_cast<uint8_t>(getMessageType()));
}

int Ump::getSizeInBytes() const {
    switch (getMessageType()) {
        case MessageType::SYSEX8_MDS:
        case MessageType::FLEX_DATA:
        case MessageType::UMP_STREAM:
            return 16;
        case MessageType::SYSEX7:
        case MessageType::MIDI2:
            return 8;
        default:
            return 4;
    }
}

BinaryChunkStatus Ump::getBinaryChunkStatus() const {
    uint8_t status = getStatusByte();
    if (status == 0x00) return BinaryChunkStatus::COMPLETE_PACKET;
    if (status == 0x10) return BinaryChunkStatus::START;
    if (status == 0x20) return BinaryChunkStatus::CONTINUE;
    if (status == 0x30) return BinaryChunkStatus::END;
    return BinaryChunkStatus::COMPLETE_PACKET;
}

bool Ump::isJRClock() const {
    return getMessageType() == MessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::JR_CLOCK;
}

uint16_t Ump::getJRClock() const {
    return isJRClock() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isJRTimestamp() const {
    return getMessageType() == MessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::JR_TIMESTAMP;
}

uint16_t Ump::getJRTimestamp() const {
    return isJRTimestamp() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isDCTPQ() const {
    return getMessageType() == MessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::DCTPQ;
}

uint16_t Ump::getDCTPQ() const {
    return isDCTPQ() ? static_cast<uint16_t>(int1 & 0xFFFF) : 0;
}

bool Ump::isDeltaClockstamp() const {
    return getMessageType() == MessageType::UTILITY &&
           getStatusCode() == MidiUtilityStatus::DELTA_CLOCKSTAMP;
}

uint32_t Ump::getDeltaClockstamp() const {
    return isDeltaClockstamp() ? (int1 & 0xFFFFF) : 0;
}

bool Ump::isStartOfClip() const {
    return getMessageType() == MessageType::UMP_STREAM && getStatusByte() == 0x20;
}

bool Ump::isEndOfClip() const {
    return getMessageType() == MessageType::UMP_STREAM && getStatusByte() == 0x21;
}

bool Ump::isTempo() const {
    return getMessageType() == MessageType::FLEX_DATA &&
           static_cast<uint8_t>(int1 & 0xFF) == FlexDataStatus::TEMPO;
}

bool Ump::isTimeSignature() const {
    return getMessageType() == MessageType::FLEX_DATA &&
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

static uint32_t getIntFromBytes_(const uint8_t* bytes, size_t offset, size_t max_size) {
    if (offset + 3 >= max_size) return 0;

    return bytes[offset] |
           (bytes[offset + 1] << 8) |
           (bytes[offset + 2] << 16) |
           (bytes[offset + 3] << 24);
}

std::vector<Ump> parseUmpsFromBytes(const uint8_t* data, size_t start, size_t length) {
    std::vector<Ump> result;
    size_t offset = start;
    const size_t end = start + length;

    while (offset < end && (offset - start + 3) < length) {
        uint32_t int1 = getIntFromBytes_(data, offset, start + length);
        MessageType msg_type = static_cast<MessageType>((int1 >> 28) & 0xF);

        size_t ump_size;
        switch (msg_type) {
            case MessageType::SYSEX8_MDS:
            case MessageType::FLEX_DATA:
            case MessageType::UMP_STREAM:
                ump_size = 16;
                break;
            case MessageType::SYSEX7:
            case MessageType::MIDI2:
                ump_size = 8;
                break;
            default:
                ump_size = 4;
                break;
        }

        if (offset + ump_size > end) break;

        uint32_t int2 = (ump_size > 4) ? getIntFromBytes_(data, offset + 4, start + length) : 0;
        uint32_t int3 = (ump_size > 8) ? getIntFromBytes_(data, offset + 8, start + length) : 0;
        uint32_t int4 = (ump_size > 12) ? getIntFromBytes_(data, offset + 12, start + length) : 0;

        result.emplace_back(int1, int2, int3, int4);
        offset += ump_size;
    }

    return result;
}

}
