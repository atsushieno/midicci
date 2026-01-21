#include <umppi/details/Midi2Track.hpp>

namespace umppi {

int Midi2Track::getTotalTicks() const {
    int total = 0;
    for (const auto& message : messages) {
        if (message.isDeltaClockstamp()) {
            total += message.getDeltaClockstamp();
        }
    }
    return total;
}

}
