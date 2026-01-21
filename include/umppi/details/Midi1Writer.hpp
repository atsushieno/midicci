#pragma once

#include <umppi/details/Midi1Music.hpp>
#include <ostream>
#include <functional>

namespace umppi {

using MetaEventWriter = std::function<int(bool, const Midi1Event&, std::vector<uint8_t>&)>;

class Midi1Writer {
private:
    std::ostream& stream_;
    MetaEventWriter metaEventWriter_;
    bool disableRunningStatus_ = false;
    uint8_t runningStatus_ = 0;

    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void write7BitEncodedInt(int value);
    int get7BitEncodedLength(int value) const;
    int getTrackDataSize(const Midi1Track& track);
    void writeTrack(const Midi1Track& track);

public:
    explicit Midi1Writer(std::ostream& stream,
                        MetaEventWriter metaEventWriter = nullptr,
                        bool disableRunningStatus = false);

    void write(const Midi1Music& music);

    static int defaultMetaEventWriter(bool onlyCountLength, const Midi1Event& event,
                                     std::vector<uint8_t>& stream);
};

void write_midi1_file(const Midi1Music& music, const std::string& filename,
                     bool disableRunningStatus = false);

}
