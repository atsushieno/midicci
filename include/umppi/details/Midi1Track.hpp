#pragma once

#include <umppi/details/Midi1Event.hpp>
#include <vector>

namespace umppi {

class Midi1Track {
public:
    std::vector<Midi1Event> events;

    Midi1Track() = default;
    explicit Midi1Track(std::vector<Midi1Event> evts) : events(std::move(evts)) {}

    int getTotalTicks() const {
        int total = 0;
        for (const auto& evt : events) {
            total += evt.deltaTime;
        }
        return total;
    }
};

} // namespace umppi
