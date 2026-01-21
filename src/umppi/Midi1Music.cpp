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

} // namespace umppi
