#pragma once

#include <cstdint>

#include "midicci/midicci.hpp"

namespace midicci::commonprofiles {

class CommonProfileDetailsStandardTarget {
public:
    const uint8_t NUM_MIDI_CHANNELS = 0;
};

class ProfileSupportLevel {
public:
    const uint8_t PARTIAL = 0;
    const uint8_t MINIMUM_REQUIRED = 1;
    const uint8_t HIGHEST_POSSIBLE = 0x7F;
};

class DefaultControlChangesProfile {
    std::vector<uint8_t> partialBytes{STANDARD_DEFINED_PROFILE, 0x21, 0, 1, 0};
    std::vector<uint8_t> fullBytes{STANDARD_DEFINED_PROFILE, 0x21, 0, 1, 1};
public:
    MidiCIProfileId profileIdForPartial{partialBytes};

    MidiCIProfileId profileId{fullBytes};
};

}