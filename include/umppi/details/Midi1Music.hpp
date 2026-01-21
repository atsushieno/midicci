#pragma once

#include <umppi/details/Midi1Track.hpp>
#include <umppi/details/DeltaTimeComputer.hpp>
#include <vector>

namespace umppi {

class Midi1Music {
public:
    static constexpr int DEFAULT_TEMPO = 500000;

    std::vector<Midi1Track> tracks;
    int deltaTimeSpec = 0;
    uint8_t format = 1;

    Midi1Music() = default;

    void addTrack(Midi1Track track) {
        tracks.push_back(std::move(track));
    }

    int getTotalTicks() const;
    int getTotalPlayTimeMilliseconds() const;
    int getTimePositionInMillisecondsForTick(int ticks) const;

    static int getSmpteTicksPerSeconds(int smfDeltaTimeSpec);
    static double getSmpteDurationInSeconds(int smfDeltaTimeSpec, int ticks,
                                           int tempo = DEFAULT_TEMPO,
                                           double tempoRatio = 1.0);
    static int getSmpteTicksForSeconds(int smfDeltaTimeSpec, double duration,
                                      int tempo = DEFAULT_TEMPO,
                                      double tempoRatio = 1.0);
    static int getSmfTempo(const uint8_t* data, size_t offset);
    static double getSmfBpm(const uint8_t* data, size_t offset);

    static std::vector<Timed<Midi1Event>> filterEvents(
        const std::vector<Midi1Event>& messages,
        std::function<bool(const Midi1Event&)> filter);

    static int getTotalPlayTimeMilliseconds(
        const std::vector<Midi1Event>& messages,
        int deltaTimeSpec);

    static int getPlayTimeMillisecondsAtTick(
        const std::vector<Midi1Event>& messages,
        int ticks,
        int deltaTimeSpec);

private:
    class SmfDeltaTimeComputer : public DeltaTimeComputer<Midi1Event> {
    public:
        int messageToDeltaTime(const Midi1Event& message) const override;
        bool isTempoMessage(const Midi1Event& message) const override;
        int getTempoValue(const Midi1Event& message) const override;
    };

    static uint8_t getActualSmpteFrameRate(uint8_t nominalFrameRate);
    static int getSmpteTicksPerSeconds(uint8_t nominalFrameRate, int ticksPerFrame);
};

} // namespace umppi
