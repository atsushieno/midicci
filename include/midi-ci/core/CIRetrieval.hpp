#pragma once

#include <vector>
#include <cstdint>
#include <utility>
#include "../core/MidiCIConstants.hpp"

namespace midi_ci {
namespace core {

class CIRetrieval {
public:
    static uint8_t get_addressing(const std::vector<uint8_t>& sysex);
    
    static DeviceDetails get_device_details(const std::vector<uint8_t>& sysex);
    
    static uint32_t get_source_muid(const std::vector<uint8_t>& sysex);
    
    static uint32_t get_destination_muid(const std::vector<uint8_t>& sysex);
    
    static uint32_t get_muid_to_invalidate(const std::vector<uint8_t>& sysex);
    
    static uint32_t get_max_sysex_size(const std::vector<uint8_t>& sysex);
    
    static std::pair<std::vector<std::vector<uint8_t>>, std::vector<std::vector<uint8_t>>> 
    get_profile_set(const std::vector<uint8_t>& sysex);
    
    static std::vector<uint8_t> get_profile_id(const std::vector<uint8_t>& sysex);
    
    static uint16_t get_profile_enabled_channels(const std::vector<uint8_t>& sysex);
    
    static uint16_t get_profile_specific_data_size(const std::vector<uint8_t>& sysex);
    
    static uint8_t get_max_property_requests(const std::vector<uint8_t>& sysex);
    
    static std::vector<uint8_t> get_property_header(const std::vector<uint8_t>& sysex);
    
    static std::vector<uint8_t> get_property_body_in_this_chunk(const std::vector<uint8_t>& sysex);
    
    static uint16_t get_property_total_chunks(const std::vector<uint8_t>& sysex);
    
    static uint16_t get_property_chunk_index(const std::vector<uint8_t>& sysex);

private:
    static std::vector<uint8_t> get_profile_id_entry(const std::vector<uint8_t>& sysex, size_t offset);
};

} // namespace core
} // namespace midi_ci
