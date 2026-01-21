#include <umppi/details/Midi1Reader.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>
#include <fstream>
#include <stdexcept>
#include <sstream>

namespace umppi {

class SmfParserException : public std::runtime_error {
public:
    explicit SmfParserException(const std::string& message)
        : std::runtime_error(message) {}
};

Midi1Reader::Midi1Reader(std::istream& stream)
    : stream_(stream) {}

Midi1Music Midi1Reader::read() {
    Midi1Music music;

    if (readByte() != 'M' || readByte() != 'T' ||
        readByte() != 'h' || readByte() != 'd') {
        throw SmfParserException("MThd is expected");
    }

    if (readInt32() != 6) {
        throw SmfParserException("Unexpected data size (should be 6)");
    }

    music.format = static_cast<uint8_t>(readInt16());
    int16_t trackCount = readInt16();
    music.deltaTimeSpec = readInt16();

    for (int16_t i = 0; i < trackCount; i++) {
        music.tracks.push_back(readTrack());
    }

    return music;
}

Midi1Track Midi1Reader::readTrack() {
    Midi1Track track;

    if (readByte() != 'M' || readByte() != 'T' ||
        readByte() != 'r' || readByte() != 'k') {
        throw SmfParserException("MTrk is expected");
    }

    int32_t trackSize = readInt32();
    currentTrackSize_ = 0;
    runningStatus_ = 0;

    while (currentTrackSize_ < trackSize) {
        int deltaTime = readVariableLength();
        track.events.push_back(readEvent(deltaTime));
    }

    if (currentTrackSize_ != trackSize) {
        throw SmfParserException("Size information mismatch");
    }

    return track;
}

Midi1Event Midi1Reader::readEvent(int deltaTime) {
    uint8_t b = peekByte();

    if (b >= 0x80) {
        runningStatus_ = readByte();
    }

    uint8_t status = runningStatus_;

    if (status == Midi1Status::SYSEX || status == Midi1Status::SYSEX_END) {
        int len = readVariableLength();
        std::vector<uint8_t> data(len);
        if (len > 0) {
            readBytes(data.data(), len);
        }
        return Midi1Event{
            deltaTime,
            std::make_shared<Midi1CompoundMessage>(status, 0, 0, std::move(data), 0, len)
        };
    }

    if (status == Midi1Status::META) {
        uint8_t metaType = readByte();
        int len = readVariableLength();
        std::vector<uint8_t> data(len);
        if (len > 0) {
            readBytes(data.data(), len);
        }
        return Midi1Event{
            deltaTime,
            std::make_shared<Midi1CompoundMessage>(status, metaType, 0, std::move(data), 0, len)
        };
    }

    int value = status;
    value |= static_cast<int>(readByte()) << 8;

    if (Midi1Message::fixedDataSize(status) == 2) {
        value |= static_cast<int>(readByte()) << 16;
    }

    return Midi1Event{
        deltaTime,
        std::make_shared<Midi1SimpleMessage>(value)
    };
}

uint8_t Midi1Reader::readByte() {
    currentTrackSize_++;
    char c;
    if (!stream_.get(c)) {
        throw SmfParserException("Insufficient stream. Failed to read a byte.");
    }
    return static_cast<uint8_t>(c);
}

int16_t Midi1Reader::readInt16() {
    uint8_t b1 = readByte();
    uint8_t b2 = readByte();
    return static_cast<int16_t>((static_cast<int>(b1) << 8) | b2);
}

int32_t Midi1Reader::readInt32() {
    uint8_t b1 = readByte();
    uint8_t b2 = readByte();
    uint8_t b3 = readByte();
    uint8_t b4 = readByte();
    return (static_cast<int32_t>(b1) << 24) |
           (static_cast<int32_t>(b2) << 16) |
           (static_cast<int32_t>(b3) << 8) |
           static_cast<int32_t>(b4);
}

int Midi1Reader::readVariableLength() {
    int v = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t b = readByte();
        v = (v << 7) | (b & 0x7F);
        if (b < 0x80) {
            return v;
        }
    }
    throw SmfParserException("Delta time specification exceeds the 4-byte limitation.");
}

void Midi1Reader::readBytes(uint8_t* buffer, size_t length) {
    currentTrackSize_ += static_cast<int>(length);
    if (!stream_.read(reinterpret_cast<char*>(buffer), length)) {
        std::ostringstream oss;
        oss << "The stream is insufficient to read " << length
            << " bytes specified in the SMF message.";
        throw SmfParserException(oss.str());
    }
}

uint8_t Midi1Reader::peekByte() {
    int c = stream_.peek();
    if (c == EOF) {
        throw SmfParserException("Insufficient stream. Failed to peek a byte.");
    }
    return static_cast<uint8_t>(c);
}

Midi1Music readMidi1File(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw SmfParserException("Failed to open file: " + filename);
    }
    Midi1Reader reader(file);
    return reader.read();
}

}
