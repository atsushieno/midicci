#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

namespace midi_ci {
namespace core {

namespace constants {

constexpr uint8_t MIDI_CI_SYSEX_START = 0xF0;
constexpr uint8_t MIDI_CI_SYSEX_END = 0xF7;
constexpr uint8_t MIDI_CI_UNIVERSAL_SYSEX_ID = 0x7E;
constexpr uint8_t MIDI_CI_SUB_ID_1 = 0x0D;

constexpr uint8_t MIDI_CI_VERSION_1_1 = 0x01;
constexpr uint8_t MIDI_CI_VERSION_1_2 = 0x02;

constexpr uint8_t DISCOVERY_INQUIRY = 0x70;
constexpr uint8_t DISCOVERY_REPLY = 0x71;
constexpr uint8_t INVALIDATE_MUID = 0x7E;
constexpr uint8_t ACK = 0x7D;
constexpr uint8_t NAK = 0x7F;

constexpr uint8_t PROFILE_INQUIRY = 0x20;
constexpr uint8_t PROFILE_INQUIRY_REPLY = 0x21;
constexpr uint8_t PROFILE_SET_ON = 0x22;
constexpr uint8_t PROFILE_SET_OFF = 0x23;
constexpr uint8_t PROFILE_ENABLED_REPORT = 0x24;
constexpr uint8_t PROFILE_DISABLED_REPORT = 0x25;
constexpr uint8_t PROFILE_ADDED_REPORT = 0x26;
constexpr uint8_t PROFILE_REMOVED_REPORT = 0x27;
constexpr uint8_t PROFILE_DETAILS_INQUIRY = 0x28;
constexpr uint8_t PROFILE_DETAILS_REPLY = 0x29;
constexpr uint8_t PROFILE_SPECIFIC_DATA = 0x2F;

constexpr uint8_t PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY = 0x30;
constexpr uint8_t PROPERTY_EXCHANGE_CAPABILITIES_REPLY = 0x31;
constexpr uint8_t PROPERTY_EXCHANGE_GET = 0x34;
constexpr uint8_t PROPERTY_EXCHANGE_GET_REPLY = 0x35;
constexpr uint8_t PROPERTY_EXCHANGE_SET = 0x36;
constexpr uint8_t PROPERTY_EXCHANGE_SET_REPLY = 0x37;
constexpr uint8_t PROPERTY_EXCHANGE_SUBSCRIPTION = 0x38;
constexpr uint8_t PROPERTY_EXCHANGE_SUBSCRIPTION_REPLY = 0x39;
constexpr uint8_t PROPERTY_EXCHANGE_NOTIFY = 0x3F;

constexpr uint32_t BROADCAST_MUID = 0x7F7F7F7F;
constexpr uint32_t FUNCTION_BLOCK_MUID = 0x10000000;
constexpr uint32_t MIDI_CI_BROADCAST_MUID_32 = 0x7F7F7F7F;

constexpr uint8_t MIDI_CI_ADDRESS_FUNCTION_BLOCK = 0x7F;
constexpr size_t MIDI_CI_COMMON_HEADER_SIZE = 13;

inline void serialize_muid_32(std::vector<uint8_t>& data, uint32_t muid) {
    data.push_back(static_cast<uint8_t>(muid & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 24) & 0xFF));
}

inline void serialize_7bit_int14(std::vector<uint8_t>& data, uint16_t value) {
    data.push_back(static_cast<uint8_t>(value & 0x7F));
    data.push_back(static_cast<uint8_t>((value >> 7) & 0x7F));
}

inline void serialize_common_header(std::vector<uint8_t>& data, uint8_t address, uint8_t sub_id_2, 
                                   uint8_t version, uint32_t source_muid, uint32_t dest_muid) {
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(address);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(sub_id_2);
    data.push_back(version);
    serialize_muid_32(data, source_muid);
    serialize_muid_32(data, dest_muid);
}

inline void serialize_property_common(std::vector<uint8_t>& data, uint8_t address, uint8_t sub_id_2,
                                     uint32_t source_muid, uint32_t dest_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header, uint16_t num_chunks,
                                     uint16_t chunk_index, const std::vector<uint8_t>& chunk_data) {
    serialize_common_header(data, address, sub_id_2, MIDI_CI_VERSION_1_2, source_muid, dest_muid);
    data.push_back(request_id);
    serialize_7bit_int14(data, static_cast<uint16_t>(header.size()));
    data.insert(data.end(), header.begin(), header.end());
    serialize_7bit_int14(data, num_chunks);
    serialize_7bit_int14(data, chunk_index);
    serialize_7bit_int14(data, static_cast<uint16_t>(chunk_data.size()));
    data.insert(data.end(), chunk_data.begin(), chunk_data.end());
}

inline std::vector<std::vector<uint8_t>> serialize_property_chunks(
    uint32_t max_chunk_size, uint8_t sub_id_2, uint32_t source_muid, uint32_t dest_muid,
    uint8_t request_id, const std::vector<uint8_t>& header, const std::vector<uint8_t>& data) {
    
    std::vector<std::vector<uint8_t>> result;
    
    if (data.empty()) {
        std::vector<uint8_t> packet;
        serialize_property_common(packet, MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
                                source_muid, dest_muid, request_id, header, 1, 1, data);
        result.push_back(std::move(packet));
        return result;
    }
    
    size_t num_chunks = (data.size() + max_chunk_size - 1) / max_chunk_size;
    for (size_t i = 0; i < num_chunks; ++i) {
        size_t start = i * max_chunk_size;
        size_t end = std::min(start + max_chunk_size, data.size());
        std::vector<uint8_t> chunk_data(data.begin() + start, data.begin() + end);
        
        std::vector<uint8_t> packet;
        serialize_property_common(packet, MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
                                source_muid, dest_muid, request_id, header,
                                static_cast<uint16_t>(num_chunks), static_cast<uint16_t>(i + 1), chunk_data);
        result.push_back(std::move(packet));
    }
    return result;
}

constexpr size_t MIDI_CI_PROFILE_ID_SIZE = 5;

enum class CISubId2 : uint8_t {
    DISCOVERY_INQUIRY = 0x70,
    DISCOVERY_REPLY = 0x71,
    ENDPOINT_MESSAGE_INQUIRY = 0x72,
    ENDPOINT_MESSAGE_REPLY = 0x73,
    INVALIDATE_MUID = 0x7E,
    ACK = 0x7D,
    NAK = 0x7F,
    PROFILE_INQUIRY = 0x20,
    PROFILE_INQUIRY_REPLY = 0x21,
    PROFILE_SET_ON = 0x22,
    PROFILE_SET_OFF = 0x23,
    PROFILE_ENABLED_REPORT = 0x24,
    PROFILE_DISABLED_REPORT = 0x25,
    PROFILE_ADDED_REPORT = 0x26,
    PROFILE_REMOVED_REPORT = 0x27,
    PROFILE_DETAILS_INQUIRY = 0x28,
    PROFILE_DETAILS_REPLY = 0x29,
    PROFILE_SPECIFIC_DATA = 0x2F,
    PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY = 0x30,
    PROPERTY_EXCHANGE_CAPABILITIES_REPLY = 0x31,
    PROPERTY_GET_DATA_INQUIRY = 0x34,
    PROPERTY_GET_DATA_REPLY = 0x35,
    PROPERTY_SET_DATA_INQUIRY = 0x36,
    PROPERTY_SET_DATA_REPLY = 0x37,
    PROPERTY_SUBSCRIPTION_INQUIRY = 0x38,
    PROPERTY_SUBSCRIPTION_REPLY = 0x39,
    PROPERTY_NOTIFY = 0x3F,
    PROCESS_INQUIRY_CAPABILITIES = 0x40,
    PROCESS_INQUIRY_CAPABILITIES_REPLY = 0x41,
    PROCESS_INQUIRY_MIDI_MESSAGE_REPORT = 0x42,
    PROCESS_INQUIRY_MIDI_MESSAGE_REPORT_REPLY = 0x43,
    PROCESS_INQUIRY_END_OF_MIDI_MESSAGE = 0x44
};

enum class MidiCISupportedCategories {
    NONE = 0,
    PROTOCOL_NEGOTIATION = 1, // Deprecated in MIDI-CI 1.2
    PROFILE_CONFIGURATION = 4,
    PROPERTY_EXCHANGE = 8,
    PROCES_INQUIRY = 16,
    // I'm inclined to say "All", but that may change in the future and it indeed did.
    // Even worse, the definition of those Three Ps had changed...
    THREE_P = PROFILE_CONFIGURATION + PROPERTY_EXCHANGE + PROCES_INQUIRY
};


} // namespace constants

} // namespace core
} // namespace midi_ci
