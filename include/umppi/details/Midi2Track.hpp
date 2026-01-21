#pragma once

#include <umppi/details/Ump.hpp>
#include <vector>

namespace umppi {

class Midi2Track {
public:
    std::vector<Ump> messages;

    Midi2Track() = default;

    int getTotalTicks() const;
};

}
