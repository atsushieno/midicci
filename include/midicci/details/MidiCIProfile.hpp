#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <cstdint>
#include <string>
#include <cstdio>

namespace midicci {

    struct MidiCIProfileId {
        std::vector<uint8_t> data;

        explicit MidiCIProfileId(const std::vector<uint8_t>& d) : data(d) {}

        bool operator==(const MidiCIProfileId& other) const {
            return data == other.data;
        }

        std::string to_string() const {
            std::string result;
            for (size_t i = 0; i < data.size(); ++i) {
                if (i > 0) result += ":";
                char hex[3];
                std::snprintf(hex, sizeof(hex), "%02X", data[i]);
                result += hex;
            }
            return result;
        }
    };

    struct MidiCIProfile {
        MidiCIProfileId profile;
        uint8_t group;
        uint8_t address;
        bool enabled;
        uint16_t num_channels_requested;

        MidiCIProfile(const MidiCIProfileId& p, uint8_t g, uint8_t a, bool e, uint16_t n)
                : profile(p), group(g), address(a), enabled(e), num_channels_requested(n) {}
    };

    struct MidiCIProfileDetails {
        MidiCIProfileId profile;
        uint8_t target;
        std::vector<uint8_t> data;

        MidiCIProfileDetails(const MidiCIProfileId& p, uint8_t t, const std::vector<uint8_t>& d)
            : profile(p), target(t), data(d) {}
    };
}
