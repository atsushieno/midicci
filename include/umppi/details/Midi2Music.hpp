#pragma once

#include <umppi/details/Midi2Track.hpp>
#include <umppi/details/DeltaTimeComputer.hpp>
#include <umppi/details/Utility.hpp>
#include <vector>

namespace umppi {

class Midi2Music {
public:
    static constexpr int DEFAULT_TEMPO = 500000;

    std::vector<Midi2Track> tracks;
    int deltaTimeSpec = 480;

    Midi2Music() = default;

    void addTrack(Midi2Track track) {
        tracks.push_back(std::move(track));
    }

    bool isSingleTrack() const { return tracks.size() == 1; }

    int getTotalTicks() const;
    int getTotalPlayTimeMilliseconds() const;
    int getTimePositionInMillisecondsForTick(int ticks) const;

    static bool isMetaEventMessageStarter(const Ump& message);
    static bool isTempoMessage(const Ump& message);
    static int getTempoValue(const Ump& message);

    static std::vector<Timed<Ump>> filterEvents(
        const std::vector<Ump>& messages,
        std::function<bool(const Ump&)> filter);

    static int getTotalPlayTimeMilliseconds(
        const std::vector<Ump>& messages,
        int deltaTimeSpec);

    static int getPlayTimeMillisecondsAtTick(
        const std::vector<Ump>& messages,
        int ticks,
        int deltaTimeSpec);

    Midi2Music mergeTracks() const;

private:
    class UmpDeltaTimeComputer : public DeltaTimeComputer<Ump> {
    public:
        int messageToDeltaTime(const Ump& message) const override;
        bool isTempoMessage(const Ump& message) const override;
        int getTempoValue(const Ump& message) const override;
    };
};

}
