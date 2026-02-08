#include <umppi/details/Midi1Music.hpp>

namespace umppi {

int Midi1Music::SmfDeltaTimeComputer::messageToDeltaTime(const Midi1Event& message) const {
    return message.deltaTime;
}

bool Midi1Music::SmfDeltaTimeComputer::isTempoMessage(const Midi1Event& message) const {
    return message.message->getStatusCode() == Midi1Status::META &&
           message.message->getMsb() == MidiMetaType::TEMPO;
}

int Midi1Music::SmfDeltaTimeComputer::getTempoValue(const Midi1Event& message) const {
    auto compound = dynamic_cast<const Midi1CompoundMessage*>(message.message.get());
    if (compound) {
        const auto& extraData = compound->getExtraData();
        size_t offset = compound->getExtraDataOffset();
        return getSmfTempo(extraData.data(), offset);
    }
    return DEFAULT_TEMPO;
}

int Midi1Music::getTotalTicks() const {
    if (format != 0 || tracks.empty()) {
        return 0;
    }
    return tracks[0].getTotalTicks();
}

int Midi1Music::getTotalPlayTimeMilliseconds() const {
    if (format != 0 || tracks.empty()) {
        return 0;
    }
    return getTotalPlayTimeMilliseconds(tracks[0].events, deltaTimeSpec);
}

int Midi1Music::getTimePositionInMillisecondsForTick(int ticks) const {
    if (format != 0 || tracks.empty()) {
        return 0;
    }
    return getPlayTimeMillisecondsAtTick(tracks[0].events, ticks, deltaTimeSpec);
}

uint8_t Midi1Music::getActualSmpteFrameRate(uint8_t nominalFrameRate) {
    return (nominalFrameRate == 29) ? 30 : nominalFrameRate;
}

int Midi1Music::getSmpteTicksPerSeconds(uint8_t nominalFrameRate, int ticksPerFrame) {
    return getActualSmpteFrameRate(nominalFrameRate) * ticksPerFrame;
}

int Midi1Music::getSmpteTicksPerSeconds(int smfDeltaTimeSpec) {
    uint8_t frameRate = static_cast<uint8_t>((-smfDeltaTimeSpec) >> 8);
    int ticksPerFrame = smfDeltaTimeSpec & 0xFF;
    return getSmpteTicksPerSeconds(frameRate, ticksPerFrame);
}

double Midi1Music::getSmpteDurationInSeconds(int smfDeltaTimeSpec, int ticks,
                                             int tempo, double tempoRatio) {
    return static_cast<double>(tempo) / 250000.0 * ticks /
           getSmpteTicksPerSeconds(smfDeltaTimeSpec) / tempoRatio;
}

int Midi1Music::getSmpteTicksForSeconds(int smfDeltaTimeSpec, double duration,
                                       int tempo, double tempoRatio) {
    return static_cast<int>(duration * tempoRatio / tempo * 250000.0 *
                           getSmpteTicksPerSeconds(smfDeltaTimeSpec));
}

int Midi1Music::getSmfTempo(const uint8_t* data, size_t offset) {
    return (toUnsigned(data[offset]) << 16) +
           (toUnsigned(data[offset + 1]) << 8) +
           toUnsigned(data[offset + 2]);
}

double Midi1Music::getSmfBpm(const uint8_t* data, size_t offset) {
    return 60000000.0 / getSmfTempo(data, offset);
}

std::vector<Timed<Midi1Event>> Midi1Music::filterEvents(
    const std::vector<Midi1Event>& messages,
    std::function<bool(const Midi1Event&)> filter)
{
    SmfDeltaTimeComputer calc;
    return calc.filterEvents(messages, filter);
}

int Midi1Music::getTotalPlayTimeMilliseconds(
    const std::vector<Midi1Event>& messages,
    int deltaTimeSpec)
{
    SmfDeltaTimeComputer calc;
    return calc.getTotalPlayTimeMilliseconds(messages, deltaTimeSpec);
}

int Midi1Music::getPlayTimeMillisecondsAtTick(
    const std::vector<Midi1Event>& messages,
    int ticks,
    int deltaTimeSpec)
{
    SmfDeltaTimeComputer calc;
    return calc.getPlayTimeMillisecondsAtTick(messages, ticks, deltaTimeSpec);
}

Midi1Music Midi1Music::mergeTracks() const {
    std::vector<Midi1Event> l;

    for (const auto& track : tracks) {
        int delta = 0;
        for (const auto& mev : track.events) {
            delta += mev.deltaTime;
            l.emplace_back(delta, mev.message);
        }
    }

    if (l.empty()) {
        Midi1Music ret;
        ret.deltaTimeSpec = deltaTimeSpec;
        ret.format = 0;
        ret.addTrack(Midi1Track());
        return ret;
    }

    std::vector<size_t> indexList;
    int prev = -1;
    for (size_t i = 0; i < l.size(); i++) {
        if (l[i].deltaTime != prev) {
            indexList.push_back(i);
            prev = l[i].deltaTime;
        }
    }

    std::sort(indexList.begin(), indexList.end(),
              [&l](size_t a, size_t b) { return l[a].deltaTime < l[b].deltaTime; });

    std::vector<Midi1Event> l2;
    for (size_t i = 0; i < indexList.size(); i++) {
        size_t idx = indexList[i];
        prev = l[idx].deltaTime;
        while (idx < l.size() && l[idx].deltaTime == prev) {
            l2.push_back(l[idx]);
            idx++;
        }
    }
    l = std::move(l2);

    int waitToNext = l[0].deltaTime;
    for (size_t i = 0; i < l.size() - 1; i++) {
        int tmp = l[i + 1].deltaTime - l[i].deltaTime;
        l[i] = Midi1Event(waitToNext, l[i].message);
        waitToNext = tmp;
    }
    l[l.size() - 1] = Midi1Event(waitToNext, l[l.size() - 1].message);

    Midi1Music music;
    music.deltaTimeSpec = deltaTimeSpec;
    music.format = 0;
    music.tracks.push_back(Midi1Track(std::move(l)));
    return music;
}

} // namespace umppi
