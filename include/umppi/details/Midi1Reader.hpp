#pragma once

#include <umppi/details/Midi1Music.hpp>
#include <istream>

namespace umppi {

class Midi1Reader {
private:
    std::istream& stream_;
    int currentTrackSize_ = 0;
    uint8_t runningStatus_ = 0;

    uint8_t readByte();
    int16_t readInt16();
    int32_t readInt32();
    int readVariableLength();
    void readBytes(uint8_t* buffer, size_t length);
    uint8_t peekByte();

    Midi1Track readTrack();
    Midi1Event readEvent(int deltaTime);

public:
    explicit Midi1Reader(std::istream& stream);

    Midi1Music read();
};

Midi1Music readMidi1File(const std::string& filename);

} // namespace umppi
