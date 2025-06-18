#pragma once

#include <cstdint>
#include <string>

namespace midi_ci {
namespace core {

struct DeviceConfig {
    uint32_t max_sysex_size;
    uint32_t max_property_chunk_size;
    std::string product_instance_id;
    uint8_t group;
    
    DeviceConfig(uint32_t max_sysex = 512, uint32_t max_chunk = 256, const std::string& prod_id = "", uint8_t group = 0)
        : max_sysex_size(max_sysex), max_property_chunk_size(max_chunk), product_instance_id(prod_id), group(group) {}
};

} // namespace core
} // namespace midi_ci
