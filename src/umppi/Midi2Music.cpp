#include <algorithm>
#include <stdexcept>
#include <umppi/details/Midi2Music.hpp>
#include <umppi/details/Common.hpp>
#include <umppi/details/UmpFactory.hpp>

namespace umppi {

int Midi2Music::UmpDeltaTimeComputer::messageToDeltaTime(const Ump& message) const {
    if (message.isDeltaClockstamp()) {
        return message.getDeltaClockstamp();
    }
    if (message.isJRTimestamp()) {
        return message.getJRTimestamp();
    }
    return 0;
}

bool Midi2Music::UmpDeltaTimeComputer::isTempoMessage(const Ump& message) const {
    return message.isTempo();
}

int Midi2Music::UmpDeltaTimeComputer::getTempoValue(const Ump& message) const {
    return Midi2Music::getTempoValue(message);
}

bool Midi2Music::isMetaEventMessageStarter(const Ump& message) {
    if (message.getMessageType() != MessageType::SYSEX8_MDS) {
        return false;
    }

    uint8_t statusCode = message.getStatusCode();
    if (statusCode != Midi2BinaryChunkStatus::COMPLETE_PACKET &&
        statusCode != Midi2BinaryChunkStatus::START) {
        return false;
    }

    return (message.int1 % 0x100) == 0 &&
           ((message.int2 >> 8) == 0) &&
           ((message.int2 & 0xFF) == 0xFF) &&
           (((message.int3 >> 16) & 0xFFFF) == 0xFFFF);
}

bool Midi2Music::isTempoMessage(const Ump& message) {
    return message.isTempo();
}

int Midi2Music::getTempoValue(const Ump& message) {
    if (!isTempoMessage(message)) {
        throw std::invalid_argument("Attempt to calculate tempo from non-tempo UMP");
    }
    return ((message.int3 & 0xFF) << 16) + ((message.int4 >> 16) & 0xFFFF);
}

int Midi2Music::getTotalTicks() const {
    if (!isSingleTrack()) {
        return 0;
    }
    return tracks[0].getTotalTicks();
}

int Midi2Music::getTotalPlayTimeMilliseconds() const {
    if (!isSingleTrack()) {
        return 0;
    }
    return getTotalPlayTimeMilliseconds(tracks[0].messages, deltaTimeSpec);
}

int Midi2Music::getTimePositionInMillisecondsForTick(int ticks) const {
    if (!isSingleTrack()) {
        return 0;
    }
    return getPlayTimeMillisecondsAtTick(tracks[0].messages, ticks, deltaTimeSpec);
}

std::vector<Timed<Ump>> Midi2Music::filterEvents(
    const std::vector<Ump>& messages,
    std::function<bool(const Ump&)> filter)
{
    UmpDeltaTimeComputer calc;
    return calc.filterEvents(messages, filter);
}

int Midi2Music::getTotalPlayTimeMilliseconds(
    const std::vector<Ump>& messages,
    int deltaTimeSpec)
{
    if (deltaTimeSpec > 0) {
        UmpDeltaTimeComputer calc;
        return calc.getTotalPlayTimeMilliseconds(messages, deltaTimeSpec);
    } else {
        int total = 0;
        for (const auto& m : messages) {
            if (m.isJRTimestamp()) {
                total += m.getJRTimestamp();
            }
        }
        return total / 31250;
    }
}

int Midi2Music::getPlayTimeMillisecondsAtTick(
    const std::vector<Ump>& messages,
    int ticks,
    int deltaTimeSpec)
{
    UmpDeltaTimeComputer calc;
    return calc.getPlayTimeMillisecondsAtTick(messages, ticks, deltaTimeSpec);
}

Midi2Music Midi2Music::mergeTracks() const {
    std::vector<std::pair<int, Ump>> l;

    bool jrTimestampShowedUp = false;

    for (const auto& track : tracks) {
        int absTime = 0;
        for (const auto& mev : track.messages) {
            if (mev.isDeltaClockstamp()) {
                if (jrTimestampShowedUp) {
                    throw std::runtime_error("The source contains both JR Timestamp and Delta Clockstamp, which is not supported.");
                }
                absTime += mev.getDeltaClockstamp();
            } else if (mev.isJRTimestamp()) {
                absTime += mev.getJRTimestamp();
                jrTimestampShowedUp = true;
            } else {
                l.emplace_back(absTime, mev);
            }
        }
    }

    if (l.empty()) {
        Midi2Music ret;
        ret.addTrack(Midi2Track());
        return ret;
    }

    std::vector<size_t> indexList;
    int prev = -1;
    for (size_t i = 0; i < l.size(); i++) {
        if (l[i].first != prev) {
            indexList.push_back(i);
            prev = l[i].first;
        }
    }

    std::sort(indexList.begin(), indexList.end(),
              [&l](size_t a, size_t b) { return l[a].first < l[b].first; });

    std::vector<std::pair<int, Ump>> l2;
    for (size_t i = 0; i < indexList.size(); i++) {
        size_t idx = indexList[i];
        int absTime = l[idx].first;
        while (idx < l.size() && l[idx].first == absTime) {
            l2.push_back(l[idx]);
            idx++;
        }
    }
    l = std::move(l2);

    std::vector<Ump> l3;
    for (size_t i = 0; i < l.size(); i++) {
        if (i + 1 < l.size()) {
            int delta = l[i + 1].first - l[i].first;
            if (delta > 0) {
                if (jrTimestampShowedUp) {
                    auto timestamps = UmpFactory::jrTimestamps(static_cast<uint64_t>(delta));
                    for (auto ts : timestamps) {
                        l3.emplace_back(ts);
                    }
                } else {
                    l3.emplace_back(UmpFactory::deltaClockstamp(delta));
                }
            }
        }
        l3.push_back(l[i].second);
    }

    Midi2Music m;
    m.deltaTimeSpec = deltaTimeSpec;
    m.tracks.push_back(Midi2Track());
    m.tracks[0].messages = std::move(l3);
    return m;
}

}
