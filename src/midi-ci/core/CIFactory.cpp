#include "midi-ci/core/CIFactory.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include <algorithm>

namespace midi_ci {
namespace core {

void CIFactory::midiCiDirectInt16At(std::vector<uint8_t>& dst, size_t offset, uint16_t value) {
    if (offset + 1 < dst.size()) {
        dst[offset] = static_cast<uint8_t>(value & 0xFF);
        dst[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    }
}

void CIFactory::midiCiDirectUint32At(std::vector<uint8_t>& dst, size_t offset, uint32_t value) {
    if (offset + 3 < dst.size()) {
        dst[offset] = static_cast<uint8_t>(value & 0xFF);
        dst[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFF);
        dst[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFF);
        dst[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFF);
    }
}

void CIFactory::midiCI7bitInt14At(std::vector<uint8_t>& dst, size_t offset, uint16_t value) {
    if (offset + 1 < dst.size()) {
        dst[offset] = static_cast<uint8_t>(value & 0x7F);
        dst[offset + 1] = static_cast<uint8_t>((value >> 7) & 0x7F);
    }
}

void CIFactory::midiCI7bitInt21At(std::vector<uint8_t>& dst, size_t offset, uint32_t value) {
    if (offset + 2 < dst.size()) {
        dst[offset] = static_cast<uint8_t>(value & 0x7F);
        dst[offset + 1] = static_cast<uint8_t>((value >> 7) & 0x7F);
        dst[offset + 2] = static_cast<uint8_t>((value >> 14) & 0x7F);
    }
}

void CIFactory::midiCI7bitInt28At(std::vector<uint8_t>& dst, size_t offset, uint32_t value) {
    if (offset + 3 < dst.size()) {
        dst[offset] = static_cast<uint8_t>(value & 0x7F);
        dst[offset + 1] = static_cast<uint8_t>((value >> 7) & 0x7F);
        dst[offset + 2] = static_cast<uint8_t>((value >> 14) & 0x7F);
        dst[offset + 3] = static_cast<uint8_t>((value >> 21) & 0x7F);
    }
}

void CIFactory::memcpy(std::vector<uint8_t>& dst, size_t dst_offset, const std::vector<uint8_t>& src, size_t count) {
    if (dst_offset + count <= dst.size() && count <= src.size()) {
        std::copy(src.begin(), src.begin() + count, dst.begin() + dst_offset);
    }
}

std::vector<uint8_t> CIFactory::midiCIMessageCommon(
    std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
    uint8_t version_and_format, uint32_t source_muid, uint32_t destination_muid) {
    
    dst.resize(std::max(dst.size(), size_t(13)));
    dst[0] = constants::MIDI_CI_UNIVERSAL_SYSEX_ID;
    dst[1] = address;
    dst[2] = constants::MIDI_CI_SUB_ID_1;
    dst[3] = sub_id_2;
    dst[4] = version_and_format;
    midiCI7bitInt28At(dst, 5, midiCI32to28(source_muid));
    midiCI7bitInt28At(dst, 9, midiCI32to28(destination_muid));
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 13);
}

void CIFactory::midiCIDiscoveryCommon(
    std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
    uint8_t version_and_format, uint32_t source_muid, uint32_t destination_muid,
    uint32_t device_manufacturer_3bytes, uint16_t device_family,
    uint16_t device_model, uint32_t software_revision,
    uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
    uint8_t initiatorOutputPathId) {
    
    dst.resize(std::max(dst.size(), size_t(32)));
    midiCIMessageCommon(dst, address, sub_id_2, version_and_format, source_muid, destination_muid);

    midiCiDirectUint32At(dst, 13, device_manufacturer_3bytes);
    midiCiDirectInt16At(dst, 16, device_family);
    midiCiDirectInt16At(dst, 18, device_model);
    midiCiDirectUint32At(dst, 20, software_revision);

    dst[24] = ci_category_supported;
    midiCiDirectUint32At(dst, 25, receivable_max_sysex_size);
    dst[29] = initiatorOutputPathId;
}

std::vector<uint8_t> CIFactory::midiCIDiscovery(
    std::vector<uint8_t>& dst, uint32_t source_muid,
    uint32_t device_manufacturer_3bytes, uint16_t device_family,
    uint16_t device_model, uint32_t software_revision,
    uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
    uint8_t initiatorOutputPathId) {
    
    midiCIDiscoveryCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK, static_cast<uint8_t>(constants::CISubId2::DISCOVERY_INQUIRY),
        constants::MIDI_CI_VERSION_1_2, source_muid, 0x7F7F7F7F,
        device_manufacturer_3bytes, device_family, device_model, software_revision,
        ci_category_supported, receivable_max_sysex_size, initiatorOutputPathId);
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 30);
}

std::vector<uint8_t> CIFactory::midiCIDiscoveryReply(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    uint32_t device_manufacturer_3bytes, uint16_t device_family,
    uint16_t device_model, uint32_t software_revision,
    uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
    uint8_t initiatorOutputPathId, uint8_t functionBlock) {
    
    midiCIDiscoveryCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::DISCOVERY_REPLY),
        constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid,
        device_manufacturer_3bytes, device_family, device_model, software_revision,
        ci_category_supported, receivable_max_sysex_size, initiatorOutputPathId);
    dst[30] = functionBlock;
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 31);

}

std::vector<uint8_t> CIFactory::midiCIPropertyCommon(
    std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
    uint32_t source_muid, uint32_t destination_muid, uint8_t request_id,
    const std::vector<uint8_t>& header, uint16_t num_chunks,
    uint16_t chunk_index, const std::vector<uint8_t>& chunk_data) {
    
    size_t required_size = 21 + header.size() + chunk_data.size();
    dst.resize(std::max(dst.size(), required_size));
    
    midiCIMessageCommon(dst, address, sub_id_2, constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    dst[13] = request_id;
    midiCI7bitInt14At(dst, 14, static_cast<uint16_t>(header.size()));
    
    if (!header.empty()) {
        memcpy(dst, 16, header, header.size());
    }
    
    size_t offset = 16 + header.size();
    midiCI7bitInt14At(dst, offset, num_chunks);
    midiCI7bitInt14At(dst, offset + 2, chunk_index);
    midiCI7bitInt14At(dst, offset + 4, static_cast<uint16_t>(chunk_data.size()));
    
    if (!chunk_data.empty()) {
        memcpy(dst, offset + 6, chunk_data, chunk_data.size());
    }
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + required_size);
}

std::vector<std::vector<uint8_t>> CIFactory::midiCIPropertyChunks(
    std::vector<uint8_t>& dst, uint32_t max_chunk_size, uint8_t sub_id_2,
    uint32_t source_muid, uint32_t destination_muid, uint8_t request_id,
    const std::vector<uint8_t>& header, const std::vector<uint8_t>& data) {
    
    std::vector<std::vector<uint8_t>> result;
    
    if (data.empty()) {
        auto packet = midiCIPropertyCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
                                         source_muid, destination_muid, request_id, header, 1, 1, data);
        result.push_back(packet);
        return result;
    }
    
    size_t num_chunks = (data.size() + max_chunk_size - 1) / max_chunk_size;
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t start = i * max_chunk_size;
        size_t end = std::min(start + max_chunk_size, data.size());
        std::vector<uint8_t> chunk_data(data.begin() + start, data.begin() + end);
        
        auto packet = midiCIPropertyCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
                                         source_muid, destination_muid, request_id, header,
                                         static_cast<uint16_t>(num_chunks), static_cast<uint16_t>(i + 1), chunk_data);
        result.push_back(packet);
    }
    return result;
}

void CIFactory::midiCIProfile(std::vector<uint8_t>& dst, size_t offset, MidiCIProfileId info) {
    memcpy(dst, offset, info.data, 5);
}

std::vector<uint8_t> CIFactory::midiCIProfileInquiry(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid) {
    
    return midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROFILE_INQUIRY),
                              constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
}

std::vector<uint8_t> CIFactory::midiCIProfileInquiryReply(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    const std::vector<MidiCIProfileId>& enabled_profiles,
    const std::vector<MidiCIProfileId>& disabled_profiles) {
    
    size_t required_size = 17 + (enabled_profiles.size() * 5) + (disabled_profiles.size() * 5);
    dst.resize(std::max(dst.size(), required_size));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROFILE_INQUIRY_REPLY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    midiCI7bitInt14At(dst, 13, static_cast<uint16_t>(enabled_profiles.size()));
    
    size_t offset = 15;
    for (const auto& profile : enabled_profiles) {
        midiCIProfile(dst, offset, profile);
        offset += 5;
    }
    
    midiCI7bitInt14At(dst, offset, static_cast<uint16_t>(disabled_profiles.size()));
    offset += 2;
    
    for (const auto& profile : disabled_profiles) {
        midiCIProfile(dst, offset, profile);
        offset += 5;
    }
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + offset);
}

std::vector<uint8_t> CIFactory::midiCIProfileSet(
    std::vector<uint8_t>& dst, uint8_t address, bool turnOn, uint32_t source_muid, uint32_t destination_muid,
    const MidiCIProfileId profile_id, uint16_t num_channels) {
    
    dst.resize(std::max(dst.size(), size_t(20)));
    
    uint8_t sub_id = turnOn ? static_cast<uint8_t>(constants::CISubId2::PROFILE_SET_ON)
                             : static_cast<uint8_t>(constants::CISubId2::PROFILE_SET_OFF);
    
    midiCIMessageCommon(dst, address, sub_id, constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    midiCIProfile(dst, 13, profile_id);

    midiCI7bitInt14At(dst, 18, num_channels);
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 20);
}

std::vector<uint8_t> CIFactory::midiCIProfileReport(
    std::vector<uint8_t>& dst, uint8_t address, bool isEnabledReport,
    uint32_t source_muid, const MidiCIProfileId profile_id, uint16_t num_channels) {
    
    dst.resize(std::max(dst.size(), size_t(20)));
    
    midiCIMessageCommon(dst, address,
                        static_cast<uint8_t>(isEnabledReport ? constants::CISubId2::PROFILE_ENABLED_REPORT : constants::CISubId2::PROFILE_DISABLED_REPORT),
                        constants::MIDI_CI_VERSION_1_2, source_muid, 0x7F7F7F7F);
    
    midiCIProfile(dst, 13, profile_id);

    midiCI7bitInt14At(dst, 18, num_channels);
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 20);
}

std::vector<uint8_t> CIFactory::midiCIProfileDetailsInquiry(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    const MidiCIProfileId profile_id, uint8_t inquiry_target) {
    
    dst.resize(std::max(dst.size(), size_t(19)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROFILE_DETAILS_INQUIRY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    midiCIProfile(dst, 13, profile_id);

    dst[18] = inquiry_target;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 19);
}

std::vector<uint8_t> CIFactory::midiCIProfileDetailsReply(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    const MidiCIProfileId profile_id, uint8_t inquiry_target,
    const std::vector<uint8_t>& data) {
    
    size_t required_size = 19 + data.size();
    dst.resize(std::max(dst.size(), required_size));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROFILE_DETAILS_REPLY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    midiCIProfile(dst, 13, profile_id);

    dst[18] = inquiry_target;
    
    if (!data.empty()) {
        memcpy(dst, 19, data, data.size());
    }
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + required_size);
}

std::vector<uint8_t> CIFactory::midiCIProfileSpecificData(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    const MidiCIProfileId profile_id, const std::vector<uint8_t>& data) {
    
    size_t required_size = 22 + data.size();
    dst.resize(std::max(dst.size(), required_size));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROFILE_SPECIFIC_DATA),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    midiCIProfile(dst, 13, profile_id);

    midiCiDirectUint32At(dst, 18, static_cast<uint32_t>(data.size()));
    
    if (!data.empty()) {
        memcpy(dst, 22, data, data.size());
    }
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + required_size);
}

std::vector<uint8_t> CIFactory::midiCIPropertyExchangeCapabilities(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    uint8_t max_simultaneous_requests) {
    
    dst.resize(std::max(dst.size(), size_t(14)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    dst[13] = max_simultaneous_requests;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 14);
}

std::vector<uint8_t> CIFactory::midiCIPropertyExchangeCapabilitiesReply(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    uint8_t max_simultaneous_requests) {
    
    dst.resize(std::max(dst.size(), size_t(14)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_REPLY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    dst[13] = max_simultaneous_requests;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 14);
}

std::vector<uint8_t> CIFactory::midiCIProcessInquiryCapabilities(
    std::vector<uint8_t>& dst, uint32_t source_muid, uint32_t destination_muid) {
    
    return midiCIMessageCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK,
                              static_cast<uint8_t>(constants::CISubId2::PROCESS_INQUIRY_CAPABILITIES),
                              constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
}

std::vector<uint8_t> CIFactory::midiCIProcessInquiryCapabilitiesReply(
    std::vector<uint8_t>& dst, uint32_t source_muid, uint32_t destination_muid,
    uint8_t supported_features) {
    
    dst.resize(std::max(dst.size(), size_t(14)));
    
    midiCIMessageCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK,
                       static_cast<uint8_t>(constants::CISubId2::PROCESS_INQUIRY_CAPABILITIES_REPLY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    dst[13] = supported_features;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 14);
}

std::vector<uint8_t> CIFactory::midiCIMidiMessageReport(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    uint8_t message_data_control, uint8_t system_messages,
    uint8_t channel_controller_messages, uint8_t note_data_messages) {
    
    dst.resize(std::max(dst.size(), size_t(18)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROCESS_INQUIRY_MIDI_MESSAGE_REPORT),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    dst[13] = message_data_control;
    dst[14] = system_messages;
    dst[15] = 0;
    dst[16] = channel_controller_messages;
    dst[17] = note_data_messages;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 18);
}

std::vector<uint8_t> CIFactory::midiCIMidiMessageReportReply(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
    uint8_t system_messages, uint8_t channel_controller_messages, uint8_t note_data_messages) {
    
    dst.resize(std::max(dst.size(), size_t(17)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROCESS_INQUIRY_MIDI_MESSAGE_REPORT_REPLY),
                       constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
    
    dst[13] = system_messages;
    dst[14] = 0;
    dst[15] = channel_controller_messages;
    dst[16] = note_data_messages;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 17);
}

std::vector<uint8_t> CIFactory::midiCIEndOfMidiMessage(
    std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid) {
    
    return midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::PROCESS_INQUIRY_END_OF_MIDI_MESSAGE),
                              constants::MIDI_CI_VERSION_1_2, source_muid, destination_muid);
}

std::vector<uint8_t> CIFactory::midiCIAckNak(
    std::vector<uint8_t>& dst, bool is_nak, uint8_t address, uint8_t version_and_format,
    uint32_t source_muid, uint32_t destination_muid, uint8_t original_sub_id,
    uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& nak_details,
    const std::vector<uint8_t>& message_text_data) {
    
    size_t required_size = 23 + message_text_data.size();
    dst.resize(std::max(dst.size(), required_size));
    
    uint8_t sub_id = is_nak ? static_cast<uint8_t>(constants::CISubId2::NAK) 
                            : static_cast<uint8_t>(constants::CISubId2::ACK);
    
    midiCIMessageCommon(dst, address, sub_id, version_and_format, source_muid, destination_muid);
    
    dst[13] = original_sub_id;
    dst[14] = status_code;
    dst[15] = status_data;
    
    if (nak_details.size() >= 5) {
        memcpy(dst, 16, nak_details, 5);
    }
    
    dst[21] = static_cast<uint8_t>(message_text_data.size() % 0x80);
    dst[22] = static_cast<uint8_t>(message_text_data.size() / 0x80);
    
    if (!message_text_data.empty()) {
        memcpy(dst, 23, message_text_data, message_text_data.size());
    }
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + required_size);
}

uint32_t CIFactory::midiCI32to28(uint32_t value) {
    return ((value >> 24) << 21) +
           (((value >> 16) & 0x7F) << 14) +
           (((value >> 8) & 0x7F) << 7) +
           (value & 0x7F);
}

std::vector<uint8_t> CIFactory::midiCIInvalidateMuid(
    std::vector<uint8_t>& dst, uint8_t version_and_format, 
    uint32_t source_muid, uint32_t target_muid) {
    
    dst.resize(std::max(dst.size(), size_t(17)));
    
    midiCIMessageCommon(dst, constants::MIDI_CI_ADDRESS_FUNCTION_BLOCK, 
                       static_cast<uint8_t>(constants::CISubId2::INVALIDATE_MUID),
                       version_and_format, source_muid, constants::MIDI_CI_BROADCAST_MUID_32);
    
    midiCI7bitInt28At(dst, 13, midiCI32to28(target_muid));
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 17);
}

std::vector<uint8_t> CIFactory::midiCIDiscoveryNak(
    std::vector<uint8_t>& dst, uint8_t address, uint8_t version_and_format,
    uint32_t source_muid, uint32_t destination_muid) {
    
    dst.resize(std::max(dst.size(), size_t(13)));
    
    midiCIMessageCommon(dst, address, static_cast<uint8_t>(constants::CISubId2::NAK),
                       version_and_format, source_muid, destination_muid);
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 13);
}

std::vector<uint8_t> CIFactory::midiCIPropertyGetCapabilities(
    std::vector<uint8_t>& dst, uint8_t address, bool is_reply,
    uint32_t source_muid, uint32_t destination_muid, uint8_t max_simultaneous_requests) {
    
    dst.resize(std::max(dst.size(), size_t(16)));
    
    uint8_t sub_id = is_reply ? static_cast<uint8_t>(constants::CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_REPLY)
                              : static_cast<uint8_t>(constants::CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY);
    
    midiCIMessageCommon(dst, address, sub_id, constants::MIDI_CI_VERSION_1_2, 
                       source_muid, destination_muid);
    
    dst[13] = max_simultaneous_requests;
    dst[14] = 0;
    dst[15] = 0;
    
    return std::vector<uint8_t>(dst.begin(), dst.begin() + 16);
}

} // namespace core
} // namespace midi_ci
