#include <umppi/details/Midi1Writer.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/Utility.hpp>
#include <fstream>
#include <stdexcept>
#include <algorithm>

namespace umppi {

Midi1Writer::Midi1Writer(std::ostream& stream,
                         MetaEventWriter metaEventWriter,
                         bool disableRunningStatus)
    : stream_(stream)
    , metaEventWriter_(metaEventWriter ? metaEventWriter : defaultMetaEventWriter)
    , disableRunningStatus_(disableRunningStatus) {}

void Midi1Writer::write(const Midi1Music& music) {
    stream_.put('M');
    stream_.put('T');
    stream_.put('h');
    stream_.put('d');

    writeInt32(6);
    writeInt16(static_cast<int16_t>(music.format));
    writeInt16(static_cast<int16_t>(music.tracks.size()));
    writeInt16(static_cast<int16_t>(music.deltaTimeSpec));

    for (const auto& track : music.tracks) {
        writeTrack(track);
    }
}

void Midi1Writer::writeTrack(const Midi1Track& track) {
    stream_.put('M');
    stream_.put('T');
    stream_.put('r');
    stream_.put('k');

    writeInt32(getTrackDataSize(track));

    runningStatus_ = 0;
    bool wroteEndOfTrack = false;

    for (const auto& event : track.events) {
        write7BitEncodedInt(event.deltaTime);

        uint8_t status = event.message->getStatusByte();
        uint8_t statusCode = event.message->getStatusCode();

        if (status == Midi1Status::META) {
            std::vector<uint8_t> buffer;
            metaEventWriter_(false, event, buffer);
            stream_.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());

            auto* compound = dynamic_cast<const Midi1CompoundMessage*>(event.message.get());
            if (compound && compound->getMsb() == MidiMetaType::END_OF_TRACK) {
                wroteEndOfTrack = true;
            }
        } else if (status == Midi1Status::SYSEX || status == Midi1Status::SYSEX_END) {
            auto* compound = dynamic_cast<const Midi1CompoundMessage*>(event.message.get());
            if (!compound) {
                throw std::runtime_error("SysEx event must be Midi1CompoundMessage");
            }

            stream_.put(static_cast<char>(status));

            const auto& data = compound->getExtraData();
            size_t offset = compound->getExtraDataOffset();
            size_t length = compound->getExtraDataLength();

            write7BitEncodedInt(static_cast<int>(length));
            if (length > 0) {
                stream_.write(reinterpret_cast<const char*>(data.data() + offset), length);
            }
        } else {
            if (disableRunningStatus_ || status != runningStatus_) {
                stream_.put(static_cast<char>(status));
            }

            int fixedSize = Midi1Message::fixedDataSize(statusCode);
            stream_.put(static_cast<char>(event.message->getMsb()));
            if (fixedSize > 1) {
                stream_.put(static_cast<char>(event.message->getLsb()));
            }
            if (fixedSize > 2) {
                throw std::runtime_error("Unexpected data size");
            }
        }

        runningStatus_ = status;
    }

    if (!wroteEndOfTrack) {
        stream_.put(0);
        stream_.put(static_cast<char>(Midi1Status::META));
        stream_.put(static_cast<char>(MidiMetaType::END_OF_TRACK));
        stream_.put(0);
    }
}

void Midi1Writer::writeInt16(int16_t value) {
    stream_.put(static_cast<char>((value >> 8) & 0xFF));
    stream_.put(static_cast<char>(value & 0xFF));
}

void Midi1Writer::writeInt32(int32_t value) {
    stream_.put(static_cast<char>((value >> 24) & 0xFF));
    stream_.put(static_cast<char>((value >> 16) & 0xFF));
    stream_.put(static_cast<char>((value >> 8) & 0xFF));
    stream_.put(static_cast<char>(value & 0xFF));
}

void Midi1Writer::write7BitEncodedInt(int value) {
    if (value == 0) {
        stream_.put(0);
        return;
    }

    std::vector<uint8_t> bytes;
    while (value > 0) {
        bytes.push_back(value & 0x7F);
        value >>= 7;
    }

    std::reverse(bytes.begin(), bytes.end());

    for (size_t i = 0; i < bytes.size(); i++) {
        uint8_t byte = bytes[i];
        if (i < bytes.size() - 1) {
            byte |= 0x80;
        }
        stream_.put(static_cast<char>(byte));
    }
}

int Midi1Writer::get7BitEncodedLength(int value) const {
    if (value < 0) {
        throw std::invalid_argument("Length must be non-negative integer");
    }
    if (value == 0) {
        return 1;
    }

    int ret = 0;
    while (value != 0) {
        ret++;
        value >>= 7;
    }
    return ret;
}

int Midi1Writer::getTrackDataSize(const Midi1Track& track) {
    int size = 0;
    uint8_t runningStatus = 0;
    bool wroteEndOfTrack = false;

    for (const auto& event : track.events) {
        size += get7BitEncodedLength(event.deltaTime);

        uint8_t status = event.message->getStatusByte();
        uint8_t statusCode = event.message->getStatusCode();

        if (status == Midi1Status::META) {
            std::vector<uint8_t> dummy;
            size += metaEventWriter_(true, event, dummy);

            auto* compound = dynamic_cast<const Midi1CompoundMessage*>(event.message.get());
            if (compound && compound->getMsb() == MidiMetaType::END_OF_TRACK) {
                wroteEndOfTrack = true;
            }
        } else if (status == Midi1Status::SYSEX || status == Midi1Status::SYSEX_END) {
            size++;

            auto* compound = dynamic_cast<const Midi1CompoundMessage*>(event.message.get());
            if (compound) {
                size_t length = compound->getExtraDataLength();
                size += get7BitEncodedLength(static_cast<int>(length));
                size += static_cast<int>(length);
            }
        } else {
            if (disableRunningStatus_ || runningStatus != status) {
                size++;
            }
            size += Midi1Message::fixedDataSize(statusCode);
        }

        runningStatus = status;
    }

    if (!wroteEndOfTrack) {
        size += 4;
    }

    return size;
}

int Midi1Writer::defaultMetaEventWriter(bool onlyCountLength, const Midi1Event& event,
                                       std::vector<uint8_t>& stream) {
    auto* msg = dynamic_cast<const Midi1CompoundMessage*>(event.message.get());
    if (!msg) {
        return 0;
    }

    const auto& extraData = msg->getExtraData();
    size_t offset = msg->getExtraDataOffset();
    size_t totalLength = msg->getExtraDataLength();

    if (onlyCountLength) {
        int repeatCount = static_cast<int>(totalLength / 0x7F);
        if (repeatCount == 0) {
            return 3 + static_cast<int>(totalLength);
        }
        int mod = static_cast<int>(totalLength % 0x7F);
        return repeatCount * (4 + 0x7F) - 1 + (mod > 0 ? 4 + mod : 0);
    }

    size_t written = 0;
    do {
        if (written > 0) {
            stream.push_back(0);
        }
        stream.push_back(Midi1Status::META);
        stream.push_back(msg->getMsb());

        size_t size = std::min<size_t>(0x7F, totalLength - written);
        stream.push_back(static_cast<uint8_t>(size));

        if (size > 0) {
            stream.insert(stream.end(),
                         extraData.begin() + offset + written,
                         extraData.begin() + offset + written + size);
        }
        written += size;
    } while (written < totalLength);

    return 0;
}

void writeMidi1File(const Midi1Music& music, const std::string& filename,
                   bool disableRunningStatus) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing: " + filename);
    }
    Midi1Writer writer(file, nullptr, disableRunningStatus);
    writer.write(music);
}

}
