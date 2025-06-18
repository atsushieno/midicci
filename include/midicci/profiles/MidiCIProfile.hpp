#pragma once

#include <vector>
#include <functional>
#include <memory>
#include <cstdint>

namespace midicci {
    namespace profiles {

        struct MidiCIProfileId {
            std::vector<uint8_t> data;

            explicit MidiCIProfileId(const std::vector<uint8_t>& d) : data(d) {}

            bool operator==(const MidiCIProfileId& other) const {
                return data == other.data;
            }

            std::string to_string() const {
                std::string result;
                for (auto byte : data) {
                    result += std::to_string(byte) + " ";
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
    }
}
