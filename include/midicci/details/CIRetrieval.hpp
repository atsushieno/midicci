#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include "midicci/midicci.hpp"

namespace midicci {

class CIRetrieval {
public:
    static uint8_t getAddressing(const std::vector<uint8_t>& sysex);
    
    static DeviceDetails getDeviceDetails(const std::vector<uint8_t>& sysex);
    
    static uint32_t getSourceMuid(const std::vector<uint8_t>& sysex);
    
    static uint32_t getDestinationMuid(const std::vector<uint8_t>& sysex);
    
    static uint32_t getMuidToInvalidate(const std::vector<uint8_t>& sysex);
    
    static uint32_t getMaxSysexSize(const std::vector<uint8_t>& sysex);
    
    static std::pair<std::vector<MidiCIProfileId>, std::vector<MidiCIProfileId>>
    getProfileSet(const std::vector<uint8_t>& sysex);
    
    static MidiCIProfileId getProfileId(const std::vector<uint8_t>& sysex);
    
    static uint16_t getProfileEnabledChannels(const std::vector<uint8_t>& sysex);
    
    static uint16_t getProfileSpecificDataSize(const std::vector<uint8_t>& sysex);
    
    static uint8_t getMaxPropertyRequests(const std::vector<uint8_t>& sysex);
    
    static std::vector<uint8_t> getPropertyHeader(const std::vector<uint8_t>& sysex);
    
    static std::vector<uint8_t> getPropertyBodyInThisChunk(const std::vector<uint8_t>& sysex);
    
    static uint16_t getPropertyTotalChunks(const std::vector<uint8_t>& sysex);
    
    static uint16_t getPropertyChunkIndex(const std::vector<uint8_t>& sysex);

private:
    static MidiCIProfileId getProfileIdEntry(const std::vector<uint8_t>& sysex, size_t offset);
};

} // namespace midi_ci
