#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace midicci {
// FIXME: should we set some specific namespace for specific MMA properties?

struct MidiCIChannel {
    std::string title;
    int channel;  // 1-256 range as per spec
    std::string program_title;
    uint8_t bank_msb;
    uint8_t bank_lsb;
    uint8_t program;
    int cluster_channel_start;  // 1-256 range as per spec
    int cluster_length;
    bool isOmniOn;
    bool isPolyMode;
    std::string cluster_type;
    
    explicit MidiCIChannel(const std::string& title = "", int channel = 1,
                 const std::string& program_title = "", uint8_t bank_msb = 0,
                 uint8_t bank_lsb = 0, uint8_t program = 0,
                 int cluster_channel_start = 1, int cluster_length = 1,
                 bool isOmniOn = true, bool isPolyMode = true,
                 const std::string& cluster_type = "other")
        : title(title), channel(channel), program_title(program_title),
          bank_msb(bank_msb), bank_lsb(bank_lsb), program(program),
          cluster_channel_start(cluster_channel_start), cluster_length(cluster_length),
          isOmniOn(isOmniOn), isPolyMode(isPolyMode), cluster_type(cluster_type) {}
          
    uint8_t getClusterMidiMode() const {
        return static_cast<uint8_t>((isOmniOn ? 1 : 0) + (isPolyMode ? 2 : 0) + 1);
    }
};

struct MidiCIChannelList {
    std::vector<MidiCIChannel> channels;
    
    MidiCIChannelList() = default;
};

} // namespace
