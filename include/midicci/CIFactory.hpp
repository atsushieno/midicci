#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include "midicci/MidiCIProfile.hpp"

namespace midicci {

class CIFactory {
public:
    static void midiCiDirectInt16At(std::vector<uint8_t>& dst, size_t offset, uint16_t value);
    static void midiCiDirectUint32At(std::vector<uint8_t>& dst, size_t offset, uint32_t value);
    static void midiCI7bitInt14At(std::vector<uint8_t>& dst, size_t offset, uint16_t value);
    static void midiCI7bitInt21At(std::vector<uint8_t>& dst, size_t offset, uint32_t value);
    static void midiCI7bitInt28At(std::vector<uint8_t>& dst, size_t offset, uint32_t value);
    
    static void memcpy(std::vector<uint8_t>& dst, size_t dst_offset, const std::vector<uint8_t>& src, size_t count);
    
    static std::vector<uint8_t> midiCIMessageCommon(
        std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
        uint8_t version_and_format, uint32_t source_muid, uint32_t destination_muid);
    
    static void midiCIDiscoveryCommon(
        std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
        uint8_t version_and_format, uint32_t source_muid, uint32_t destination_muid,
        uint32_t device_manufacturer_3bytes, uint16_t device_family,
        uint16_t device_model, uint32_t software_revision,
        uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
        uint8_t initiatorOutputPathId);
    
    static std::vector<uint8_t> midiCIDiscovery(
        std::vector<uint8_t>& dst, uint32_t source_muid,
        uint32_t device_manufacturer, uint16_t device_family,
        uint16_t device_model, uint32_t software_revision,
        uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
        uint8_t initiatorOutputPathId);
    
    static std::vector<uint8_t> midiCIDiscoveryReply(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        uint32_t device_manufacturer_3bytes, uint16_t device_family,
        uint16_t device_model, uint32_t software_revision,
        uint8_t ci_category_supported, uint32_t receivable_max_sysex_size,
        uint8_t initiatorOutputPathId, uint8_t functionBlock);
    
    static std::vector<uint8_t> midiCIPropertyCommon(
        std::vector<uint8_t>& dst, uint8_t address, uint8_t sub_id_2,
        uint32_t source_muid, uint32_t destination_muid, uint8_t request_id,
        const std::vector<uint8_t>& header, uint16_t num_chunks,
        uint16_t chunk_index, const std::vector<uint8_t>& chunk_data);
    
    static std::vector<uint8_t> midiCIPropertyPacketCommon(
        std::vector<uint8_t>& dst, uint8_t sub_id_2, uint32_t source_muid, uint32_t destination_muid,
        uint8_t request_id, const std::vector<uint8_t>& header, uint16_t num_chunks,
        uint16_t chunk_index, const std::vector<uint8_t>& data);
    
    static std::vector<std::vector<uint8_t>> midiCIPropertyChunks(
        std::vector<uint8_t>& dst, uint32_t max_chunk_size, uint8_t sub_id_2,
        uint32_t source_muid, uint32_t destination_muid, uint8_t request_id,
        const std::vector<uint8_t>& header, const std::vector<uint8_t>& data);

    static void midiCIProfile(std::vector<uint8_t>& dst, size_t offset, MidiCIProfileId info);

    static std::vector<uint8_t> midiCIProfileInquiry(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid);
    
    static std::vector<uint8_t> midiCIProfileInquiryReply(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        const std::vector<MidiCIProfileId>& enabled_profiles,
        const std::vector<MidiCIProfileId>& disabled_profiles);
    
    static std::vector<uint8_t> midiCIProfileSet(
        std::vector<uint8_t>& dst, uint8_t address, bool turnOn, uint32_t source_muid, uint32_t destination_muid,
        const MidiCIProfileId profile_id, uint16_t num_channels);
    
    static std::vector<uint8_t> midiCIProfileReport(
        std::vector<uint8_t>& dst, uint8_t address, bool isEnabledReport,
        uint32_t source_muid, const MidiCIProfileId profile_id, uint16_t num_channels);
    
    static std::vector<uint8_t> midiCIProfileDetailsInquiry(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        const MidiCIProfileId profile_id, uint8_t inquiry_target);
    
    static std::vector<uint8_t> midiCIProfileDetailsReply(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        const MidiCIProfileId profile_id, uint8_t inquiry_target,
        const std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> midiCIProfileSpecificData(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        const MidiCIProfileId profile_id, const std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> midiCIPropertyExchangeCapabilities(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        uint8_t max_simultaneous_requests);
    
    static std::vector<uint8_t> midiCIPropertyExchangeCapabilitiesReply(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        uint8_t max_simultaneous_requests);
    
    static std::vector<uint8_t> midiCIProcessInquiryCapabilities(
        std::vector<uint8_t>& dst, uint32_t source_muid, uint32_t destination_muid);
    
    static std::vector<uint8_t> midiCIProcessInquiryCapabilitiesReply(
        std::vector<uint8_t>& dst, uint32_t source_muid, uint32_t destination_muid,
        uint8_t supported_features);
    
    static std::vector<uint8_t> midiCIMidiMessageReport(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        uint8_t message_data_control, uint8_t system_messages,
        uint8_t channel_controller_messages, uint8_t note_data_messages);
    
    static std::vector<uint8_t> midiCIMidiMessageReportReply(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid,
        uint8_t system_messages, uint8_t channel_controller_messages, uint8_t note_data_messages);
    
    static std::vector<uint8_t> midiCIEndOfMidiMessage(
        std::vector<uint8_t>& dst, uint8_t address, uint32_t source_muid, uint32_t destination_muid);
    
    static std::vector<uint8_t> midiCIEndpointMessage(
        std::vector<uint8_t>& dst, uint8_t version_and_format, uint32_t source_muid, 
        uint32_t destination_muid, uint8_t status);
    
    static std::vector<uint8_t> midiCIEndpointMessageReply(
        std::vector<uint8_t>& dst, uint8_t version_and_format, uint32_t source_muid, 
        uint32_t destination_muid, uint8_t status, const std::vector<uint8_t>& information_data);
    
    static std::vector<uint8_t> midiCIProfileAddedRemoved(
        std::vector<uint8_t>& dst, uint8_t address, bool is_removed, 
        uint32_t source_muid, const MidiCIProfileId& profile_id);
    
    static std::vector<uint8_t> midiCIAckNak(
        std::vector<uint8_t>& dst, bool is_nak, uint8_t address, uint8_t version_and_format,
        uint32_t source_muid, uint32_t destination_muid, uint8_t original_sub_id,
        uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& nak_details,
        const std::vector<uint8_t>& message_text_data);
    
    static uint32_t midiCI32to28(uint32_t value);
    
    static std::vector<uint8_t> midiCIInvalidateMuid(
        std::vector<uint8_t>& dst, uint8_t version_and_format, 
        uint32_t source_muid, uint32_t target_muid);
    
    static std::vector<uint8_t> midiCIDiscoveryNak(
        std::vector<uint8_t>& dst, uint8_t address, uint8_t version_and_format,
        uint32_t source_muid, uint32_t destination_muid);
    
    static std::vector<uint8_t> midiCIPropertyGetCapabilities(
        std::vector<uint8_t>& dst, uint8_t address, bool is_reply,
        uint32_t source_muid, uint32_t destination_muid, uint8_t max_simultaneous_requests);
};

} // namespace midi_ci
