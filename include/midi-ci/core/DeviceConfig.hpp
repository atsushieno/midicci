#pragma once

#include <cstdint>

namespace midi_ci {
namespace core {

struct DeviceConfig {
    uint32_t max_sysex_size;
    uint32_t max_property_chunk_size;
    
    DeviceConfig(uint32_t max_sysex = 512, uint32_t max_chunk = 256) 
        : max_sysex_size(max_sysex), max_property_chunk_size(max_chunk) {}
};

} // namespace core
} // namespace midi_ci
