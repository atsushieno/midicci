#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace midicci {

struct DeviceDetails {
    uint32_t manufacturer;
    uint16_t family;
    uint16_t modelNumber;
    uint32_t softwareRevisionLevel;

    explicit DeviceDetails(uint32_t mfg = 0, uint16_t fam = 0, uint16_t model = 0, uint32_t version = 0)
            : manufacturer(mfg), family(fam), modelNumber(model), softwareRevisionLevel(version) {}
};

struct DeviceInfo {
    uint32_t manufacturer_id;
    uint16_t family_id;
    uint16_t model_id;
    uint32_t version_id;
    std::string manufacturer;
    std::string family;
    std::string model;
    std::string version;
    std::string serial_number;

    DeviceInfo(uint32_t manufacturer_id, uint16_t family_id, uint16_t model_id, uint32_t version_id,
               const std::string& mfg, const std::string& fam, const std::string& mod, const std::string& ver,
               const std::string& serial_number)
            : manufacturer_id(manufacturer_id), family_id(family_id), model_id(model_id), version_id(version_id),
              manufacturer(mfg), family(fam), model(mod), version(ver), serial_number(serial_number) {}
};

constexpr uint8_t UNIVERSAL_SYSEX = 0x7E;
constexpr uint8_t SYSEX_SUB_ID_MIDI_CI = 0x0D;

constexpr uint8_t CI_VERSION_AND_FORMAT = 0x2;
constexpr uint8_t PROPERTY_EXCHANGE_MAJOR_VERSION = 0;
constexpr uint8_t PROPERTY_EXCHANGE_MINOR_VERSION = 0;

constexpr uint8_t ENDPOINT_STATUS_PRODUCT_INSTANCE_ID = 0;

// It seems there are some uncomfortable facts around the stable buffer sizes...
#if defined(__APPLE__)
constexpr uint32_t DEFAULT_RECEIVABLE_MAX_SYSEX_SIZE = 4096;
constexpr uint32_t DEFAULT_MAX_PROPERTY_CHUNK_SIZE = 4096 - 256;
#else
constexpr uint32_t DEFAULT_RECEIVABLE_MAX_SYSEX_SIZE = 2048;
constexpr uint32_t DEFAULT_MAX_PROPERTY_CHUNK_SIZE = 2048 - 256;
#endif
constexpr uint8_t DEFAULT_MAX_SIMULTANEOUS_PROPERTY_REQUESTS = 127;

constexpr uint8_t ADDRESS_GROUP = 0x7E;
constexpr uint8_t ADDRESS_FUNCTION_BLOCK = 0x7F;

constexpr uint8_t NO_FUNCTION_BLOCK = 0x7F;
constexpr uint8_t WHOLE_FUNCTION_BLOCK = 0x7F;

constexpr uint32_t BROADCAST_MUID_28 = 0xFFFFFFF;
constexpr uint32_t BROADCAST_MUID_32 = 0x7F7F7F7F;

constexpr uint8_t STANDARD_DEFINED_PROFILE = 0x7E;

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

// NAK Status Codes
constexpr uint8_t CI_NAK_STATUS_NAK = 0;
constexpr uint8_t CI_NAK_STATUS_MESSAGE_NOT_SUPPORTED = 1;
constexpr uint8_t CI_NAK_STATUS_CI_VERSION_NOT_SUPPORTED = 2;
constexpr uint8_t CI_NAK_STATUS_TARGET_NOT_IN_USE = 3;
constexpr uint8_t CI_NAK_STATUS_PROFILE_NOT_SUPPORTED_ON_TARGET = 4;
constexpr uint8_t CI_NAK_STATUS_TERMINATE_INQUIRY = 0x20;
constexpr uint8_t CI_NAK_STATUS_PROPERTY_EXCHANGE_CHUNKS_OUT_OF_SEQUENCE = 0x21;
constexpr uint8_t CI_NAK_STATUS_ERROR_RETRY_SUGGESTED = 0x40;
constexpr uint8_t CI_NAK_STATUS_MALFORMED_MESSAGE = 0x41;
constexpr uint8_t CI_NAK_STATUS_TIMEOUT = 0x42;
constexpr uint8_t CI_NAK_STATUS_TIMEOUT_RETRY_SUGGESTED = 0x43;

inline void serializeMuid32(std::vector<uint8_t>& data, uint32_t muid) {
    data.push_back(static_cast<uint8_t>(muid & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((muid >> 24) & 0xFF));
}

inline void serialize7bitInt14(std::vector<uint8_t>& data, uint16_t value) {
    data.push_back(static_cast<uint8_t>(value & 0x7F));
    data.push_back(static_cast<uint8_t>((value >> 7) & 0x7F));
}

inline void serializeCommonHeader(std::vector<uint8_t>& data, uint8_t address, uint8_t sub_id_2,
                                   uint8_t version, uint32_t source_muid, uint32_t dest_muid) {
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(address);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(sub_id_2);
    data.push_back(version);
    serializeMuid32(data, source_muid);
    serializeMuid32(data, dest_muid);
}

inline void serializePropertyCommon(std::vector<uint8_t>& data, uint8_t address, uint8_t sub_id_2,
                                     uint32_t source_muid, uint32_t dest_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header, uint16_t num_chunks,
                                     uint16_t chunk_index, const std::vector<uint8_t>& chunk_data) {
    serializeCommonHeader(data, address, sub_id_2, MIDI_CI_VERSION_1_2, source_muid, dest_muid);
    data.push_back(request_id);
    serialize7bitInt14(data, static_cast<uint16_t>(header.size()));
    data.insert(data.end(), header.begin(), header.end());
    serialize7bitInt14(data, num_chunks);
    serialize7bitInt14(data, chunk_index);
    serialize7bitInt14(data, static_cast<uint16_t>(chunk_data.size()));
    data.insert(data.end(), chunk_data.begin(), chunk_data.end());
}

inline std::vector<std::vector<uint8_t>> serializePropertyChunks(
    uint32_t max_chunk_size, uint8_t sub_id_2, uint32_t source_muid, uint32_t dest_muid,
    uint8_t request_id, const std::vector<uint8_t>& header, const std::vector<uint8_t>& data) {

    std::vector<std::vector<uint8_t>> result;

    if (data.empty()) {
        std::vector<uint8_t> packet;
        serializePropertyCommon(packet, MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
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
        serializePropertyCommon(packet, MIDI_CI_ADDRESS_FUNCTION_BLOCK, sub_id_2,
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

// Process Inquiry Features
enum class MidiCIProcessInquiryFeatures : uint8_t {
    MIDI_MESSAGE_REPORT = 1
};

// MIDI Message Report Data Control
#undef None // X11 WTF
enum class MidiMessageReportDataControl : uint8_t {
    None = 0,
    OnlyNonDefaults = 1,
    Full = 0x7F
};

// MIDI Message Report System Messages Flags
enum class MidiMessageReportSystemMessagesFlags : uint8_t {
    MtcQuarterFrame = 1,
    SongPosition = 2,
    SongSelect = 4,
    All = 7
};

// MIDI Message Report Channel Controller Flags
enum class MidiMessageReportChannelControllerFlags : uint8_t {
    Pitchbend = 1,
    CC = 2,
    Rpn = 4,
    Nrpn = 8,
    Program = 16,
    CAf = 32,
    All = 63
};

// MIDI Message Report Note Data Flags
enum class MidiMessageReportNoteDataFlags : uint8_t {
    Notes = 1,
    PAf = 2,
    Pitchbend = 4,
    RegisteredController = 8,
    AssignableController = 16,
    All = 31
};

} // namespace midi_ci
