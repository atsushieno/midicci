#include "midi-ci/core/CIRetrieval.hpp"
#include <algorithm>

namespace midi_ci {
namespace core {

uint8_t CIRetrieval::get_addressing(const std::vector<uint8_t>& sysex) {
    return sysex.size() > 4 ? sysex[4] : 0;
}

DeviceDetails CIRetrieval::get_device_details(const std::vector<uint8_t>& sysex) {
    DeviceDetails details;
    details.manufacturer = sysex[13] | (sysex[14] << 8) | (sysex[15] << 16);
    details.family = static_cast<uint16_t>(sysex[16] | (sysex[17] << 8));
    details.modelNumber = static_cast<uint16_t>(sysex[18] | (sysex[19] << 8));
    details.softwareRevisionLevel = sysex[20] | (sysex[21] << 8) | (sysex[22] << 16) | (sysex[23] << 24);
    return details;
}

uint32_t CIRetrieval::get_source_muid(const std::vector<uint8_t>& sysex) {
    return sysex.size() > 8 ? 
        sysex[5] | (sysex[6] << 8) | (sysex[7] << 16) | (sysex[8] << 24) : 0;
}

uint32_t CIRetrieval::get_destination_muid(const std::vector<uint8_t>& sysex) {
    return sysex.size() > 12 ? 
        sysex[9] | (sysex[10] << 8) | (sysex[11] << 16) | (sysex[12] << 24) : 0;
}

uint32_t CIRetrieval::get_muid_to_invalidate(const std::vector<uint8_t>& sysex) {
    return sysex[13] | (sysex[14] << 8) | (sysex[15] << 16) | (sysex[16] << 24);
}

uint32_t CIRetrieval::get_max_sysex_size(const std::vector<uint8_t>& sysex) {
    return sysex[25] | (sysex[26] << 8) | (sysex[27] << 16) | (sysex[28] << 24);
}

std::pair<std::vector<std::vector<uint8_t>>, std::vector<std::vector<uint8_t>>> 
CIRetrieval::get_profile_set(const std::vector<uint8_t>& sysex) {
    std::vector<std::vector<uint8_t>> enabled_profiles;
    std::vector<std::vector<uint8_t>> disabled_profiles;

    if (sysex.size() < 15) {
        return {enabled_profiles, disabled_profiles};
    }

    uint16_t num_enabled = sysex[13] | (sysex[14] << 7);
    size_t pos = 15;

    for (uint16_t i = 0; i < num_enabled && pos + 5 <= sysex.size(); ++i) {
        enabled_profiles.push_back(get_profile_id_entry(sysex, pos));
        pos += 5;
    }

    if (pos + 2 <= sysex.size()) {
        uint16_t num_disabled = sysex[pos] | (sysex[pos + 1] << 7);
        pos += 2;

        for (uint16_t i = 0; i < num_disabled && pos + 5 <= sysex.size(); ++i) {
            disabled_profiles.push_back(get_profile_id_entry(sysex, pos));
            pos += 5;
        }
    }

    return {enabled_profiles, disabled_profiles};
}

std::vector<uint8_t> CIRetrieval::get_profile_id(const std::vector<uint8_t>& sysex) {
    return get_profile_id_entry(sysex, 13);
}

uint16_t CIRetrieval::get_profile_enabled_channels(const std::vector<uint8_t>& sysex) {
    return static_cast<uint16_t>(sysex[18] | (sysex[19] << 7));
}

std::vector<uint8_t> CIRetrieval::get_profile_id_entry(const std::vector<uint8_t>& sysex, size_t offset) {
    std::vector<uint8_t> profile_id;
    if (offset + 5 <= sysex.size()) {
        profile_id.assign(sysex.begin() + offset, sysex.begin() + offset + 5);
    }
    return profile_id;
}

uint16_t CIRetrieval::get_profile_specific_data_size(const std::vector<uint8_t>& sysex) {
    return static_cast<uint16_t>(sysex[19] | (sysex[20] << 7));
}

uint8_t CIRetrieval::get_max_property_requests(const std::vector<uint8_t>& sysex) {
    return sysex[13];
}

std::vector<uint8_t> CIRetrieval::get_property_header(const std::vector<uint8_t>& sysex) {
    uint16_t size = sysex[14] | (sysex[15] << 7);
    std::vector<uint8_t> header;
    if (16 + size <= sysex.size()) {
        header.assign(sysex.begin() + 16, sysex.begin() + 16 + size);
    }
    return header;
}

std::vector<uint8_t> CIRetrieval::get_property_body_in_this_chunk(const std::vector<uint8_t>& sysex) {
    uint16_t header_size = sysex[14] | (sysex[15] << 7);
    size_t index = 20 + header_size;
    if (index + 2 <= sysex.size()) {
        uint16_t body_size = sysex[index] | (sysex[index + 1] << 7);
        std::vector<uint8_t> body;
        if (22 + header_size + body_size <= sysex.size()) {
            body.assign(sysex.begin() + 22 + header_size, sysex.begin() + 22 + header_size + body_size);
        }
        return body;
    }
    return {};
}

uint16_t CIRetrieval::get_property_total_chunks(const std::vector<uint8_t>& sysex) {
    uint16_t header_size = sysex[14] | (sysex[15] << 7);
    size_t index = 16 + header_size;
    return static_cast<uint16_t>(sysex[index] | (sysex[index + 1] << 7));
}

uint16_t CIRetrieval::get_property_chunk_index(const std::vector<uint8_t>& sysex) {
    uint16_t header_size = sysex[14] | (sysex[15] << 7);
    size_t index = 18 + header_size;
    return static_cast<uint16_t>(sysex[index] | (sysex[index + 1] << 7));
}

} // namespace core
} // namespace midi_ci
