#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include "midi-ci/json/Json.hpp"
#include <sstream>
#include <iomanip>

namespace midi_ci {
namespace messages {

Message::Message(MessageType type, const Common& common)
    : type_(type), common_(common) {}

MessageType Message::get_type() const noexcept {
    return type_;
}

uint32_t Message::get_source_muid() const noexcept {
    return common_.source_muid;
}

uint32_t Message::get_destination_muid() const noexcept {
    return common_.destination_muid;
}

SinglePacketMessage::SinglePacketMessage(MessageType type, const Common& common)
    : Message(type, common) {}

MultiPacketMessage::MultiPacketMessage(MessageType type, const Common& common)
    : Message(type, common) {}

DiscoveryInquiry::DiscoveryInquiry(const Common& common, const DeviceInfo& device_info, 
                                 uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id)
    : SinglePacketMessage(MessageType::DiscoveryInquiry, common), device_info_(device_info),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), output_path_id_(output_path_id) {}

std::vector<uint8_t> DiscoveryInquiry::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(64);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (int i = 0; i < 3; ++i) data.push_back(0x00);
    for (int i = 0; i < 2; ++i) data.push_back(0x00);
    for (int i = 0; i < 2; ++i) data.push_back(0x00);
    for (int i = 0; i < 4; ++i) data.push_back(0x00);
    data.push_back(supported_features_);
    
    data.push_back(static_cast<uint8_t>(max_sysex_size_ & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 21) & 0x7F));
    
    data.push_back(output_path_id_);
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool DiscoveryInquiry::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 32;
}

std::string DiscoveryInquiry::get_label() const {
    return "DiscoveryInquiry";
}

std::string DiscoveryInquiry::get_body_string() const {
    std::ostringstream oss;
    oss << "manufacturer=" << device_info_.manufacturer 
        << ", family=" << device_info_.family
        << ", model=" << device_info_.model
        << ", version=" << device_info_.version
        << ", features=" << std::hex << static_cast<int>(supported_features_)
        << ", maxSysEx=" << max_sysex_size_
        << ", outputPath=" << static_cast<int>(output_path_id_);
    return oss.str();
}

DiscoveryReply::DiscoveryReply(const Common& common, const DeviceInfo& device_info, 
                              uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id, uint8_t function_block)
    : SinglePacketMessage(MessageType::DiscoveryReply, common), device_info_(device_info),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), 
      output_path_id_(output_path_id), function_block_(function_block) {}

std::vector<uint8_t> DiscoveryReply::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(64);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (int i = 0; i < 3; ++i) data.push_back(0x00);
    for (int i = 0; i < 2; ++i) data.push_back(0x00);
    for (int i = 0; i < 2; ++i) data.push_back(0x00);
    for (int i = 0; i < 4; ++i) data.push_back(0x00);
    data.push_back(supported_features_);
    
    data.push_back(static_cast<uint8_t>(max_sysex_size_ & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((max_sysex_size_ >> 21) & 0x7F));
    
    data.push_back(output_path_id_);
    data.push_back(function_block_);
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool DiscoveryReply::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 33;
}

std::string DiscoveryReply::get_label() const {
    return "DiscoveryReply";
}

std::string DiscoveryReply::get_body_string() const {
    std::ostringstream oss;
    oss << "manufacturer=" << device_info_.manufacturer 
        << ", family=" << device_info_.family
        << ", model=" << device_info_.model
        << ", version=" << device_info_.version
        << ", features=" << std::hex << static_cast<int>(supported_features_)
        << ", maxSysEx=" << max_sysex_size_
        << ", outputPath=" << static_cast<int>(output_path_id_)
        << ", functionBlock=" << static_cast<int>(function_block_);
    return oss.str();
}

SetProfileOn::SetProfileOn(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::SetProfileOn, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> SetProfileOn::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(static_cast<uint8_t>(num_channels_ & 0x7F));
    data.push_back(static_cast<uint8_t>((num_channels_ >> 7) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool SetProfileOn::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 22;
}

std::string SetProfileOn::get_label() const {
    return "SetProfileOn";
}

std::string SetProfileOn::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

PropertyGetCapabilities::PropertyGetCapabilities(const Common& common, uint8_t max_simultaneous_requests)
    : SinglePacketMessage(MessageType::PropertyGetCapabilities, common), max_simultaneous_requests_(max_simultaneous_requests) {}

std::vector<uint8_t> PropertyGetCapabilities::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(16);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(max_simultaneous_requests_);
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool PropertyGetCapabilities::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 16;
}

std::string PropertyGetCapabilities::get_label() const {
    return "PropertyGetCapabilities";
}

std::string PropertyGetCapabilities::get_body_string() const {
    std::ostringstream oss;
    oss << "maxSimultaneousRequests=" << static_cast<int>(max_simultaneous_requests_);
    return oss.str();
}

GetPropertyData::GetPropertyData(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header)
    : MultiPacketMessage(MessageType::GetPropertyData, common), request_id_(request_id), header_(header) {}

GetPropertyData::GetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                const std::string& res_id)
    : MultiPacketMessage(MessageType::GetPropertyData, common), request_id_(request_id) {
    header_ = create_json_header(resource_identifier, res_id, "", false, 0, 0);
}

std::vector<uint8_t> GetPropertyData::create_json_header(const std::string& resource_identifier, 
                                                        const std::string& res_id, 
                                                        const std::string& mutual_encoding,
                                                        bool set_partial,
                                                        int offset, 
                                                        int limit) const {
    using namespace midi_ci::json;
    
    JsonValue header_json = JsonValue::empty_object();
    header_json["resource"] = JsonValue(resource_identifier);
    
    if (!res_id.empty()) {
        header_json["resId"] = JsonValue(res_id);
    }
    if (!mutual_encoding.empty()) {
        header_json["mutualEncoding"] = JsonValue(mutual_encoding);
    }
    if (set_partial) {
        header_json["setPartial"] = JsonValue(set_partial);
    }
    if (offset > 0) {
        header_json["offset"] = JsonValue(offset);
    }
    if (limit > 0) {
        header_json["limit"] = JsonValue(limit);
    }
    
    return header_json.get_serialized_bytes();
}

std::vector<std::vector<uint8_t>> GetPropertyData::serialize_multi() const {
    const uint32_t max_chunk_size = 256;
    
    if (header_.size() <= max_chunk_size) {
        return {serialize()};
    }
    
    std::vector<std::vector<uint8_t>> packets;
    size_t total_chunks = (header_.size() + max_chunk_size - 1) / max_chunk_size;
    
    for (size_t chunk_idx = 0; chunk_idx < total_chunks; ++chunk_idx) {
        size_t start_pos = chunk_idx * max_chunk_size;
        size_t chunk_size = std::min(static_cast<size_t>(max_chunk_size), header_.size() - start_pos);
        
        std::vector<uint8_t> chunk_data(header_.begin() + start_pos, header_.begin() + start_pos + chunk_size);
        
        using namespace midi_ci::core::constants;
        std::vector<uint8_t> packet;
        packet.reserve(32 + chunk_data.size());
        
        packet.push_back(MIDI_CI_SYSEX_START);
        packet.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
        packet.push_back(0x7F);
        packet.push_back(MIDI_CI_SUB_ID_1);
        packet.push_back(static_cast<uint8_t>(type_));
        packet.push_back(MIDI_CI_VERSION_1_2);
        
        packet.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
        
        packet.push_back(request_id_);
        
        packet.push_back(static_cast<uint8_t>(total_chunks & 0x7F));
        packet.push_back(static_cast<uint8_t>((total_chunks >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_idx + 1) & 0x7F));
        packet.push_back(static_cast<uint8_t>(((chunk_idx + 1) >> 7) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(chunk_data.size() & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_data.size() >> 7) & 0x7F));
        
        packet.insert(packet.end(), chunk_data.begin(), chunk_data.end());
        
        packet.push_back(MIDI_CI_SYSEX_END);
        
        packets.push_back(std::move(packet));
    }
    
    return packets;
}

std::vector<std::vector<uint8_t>> SetPropertyData::serialize_multi() const {
    const uint32_t max_chunk_size = 256;
    
    if (body_.size() <= max_chunk_size) {
        return {serialize()};
    }
    
    std::vector<std::vector<uint8_t>> packets;
    size_t total_chunks = (body_.size() + max_chunk_size - 1) / max_chunk_size;
    
    for (size_t chunk_idx = 0; chunk_idx < total_chunks; ++chunk_idx) {
        size_t start_pos = chunk_idx * max_chunk_size;
        size_t chunk_size = std::min(static_cast<size_t>(max_chunk_size), body_.size() - start_pos);
        
        std::vector<uint8_t> chunk_data(body_.begin() + start_pos, body_.begin() + start_pos + chunk_size);
        
        using namespace midi_ci::core::constants;
        std::vector<uint8_t> packet;
        packet.reserve(32 + header_.size() + chunk_data.size());
        
        packet.push_back(0x7E);
        packet.push_back(0x7F);
        packet.push_back(0x0D);
        packet.push_back(static_cast<uint8_t>(type_));
        packet.push_back(0x02);
        
        packet.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
        
        packet.push_back(request_id_);
        
        packet.push_back(static_cast<uint8_t>(header_.size() & 0x7F));
        packet.push_back(static_cast<uint8_t>((header_.size() >> 7) & 0x7F));
        
        packet.insert(packet.end(), header_.begin(), header_.end());
        
        packet.push_back(static_cast<uint8_t>(total_chunks & 0x7F));
        packet.push_back(static_cast<uint8_t>((total_chunks >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_idx + 1) & 0x7F));
        packet.push_back(static_cast<uint8_t>(((chunk_idx + 1) >> 7) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(chunk_data.size() & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_data.size() >> 7) & 0x7F));
        
        packet.insert(packet.end(), chunk_data.begin(), chunk_data.end());
        
        packets.push_back(std::move(packet));
    }
    
    return packets;
}

std::vector<uint8_t> SetPropertyData::create_json_header(const std::string& resource_identifier, const std::string& res_id, 
                                                        const std::string& mutual_encoding, bool set_partial, 
                                                        int offset, int limit) const {
    using namespace midi_ci::json;
    
    JsonValue header_json = JsonValue::empty_object();
    header_json["resource"] = JsonValue(resource_identifier);
    
    if (!res_id.empty()) {
        header_json["resId"] = JsonValue(res_id);
    }
    if (!mutual_encoding.empty()) {
        header_json["mutualEncoding"] = JsonValue(mutual_encoding);
    }
    if (set_partial) {
        header_json["setPartial"] = JsonValue(true);
    }
    if (offset > 0) {
        header_json["offset"] = JsonValue(offset);
    }
    if (limit > 0) {
        header_json["limit"] = JsonValue(limit);
    }
    
    return header_json.get_serialized_bytes();
}

std::vector<uint8_t> GetPropertyData::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32 + header_.size());
    
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x00);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(request_id_);
    
    data.insert(data.end(), header_.begin(), header_.end());
    
    return data;
}

bool GetPropertyData::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 18;
}

std::string GetPropertyData::get_label() const {
    return "GetPropertyData";
}

std::string GetPropertyData::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", headerSize=" << header_.size();
    return oss.str();
}

SetPropertyData::SetPropertyData(const Common& common, uint8_t request_id, 
                                const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : MultiPacketMessage(MessageType::SetPropertyData, common), request_id_(request_id), header_(header), body_(body) {}

SetPropertyData::SetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                const std::vector<uint8_t>& body, const std::string& res_id, bool set_partial)
    : MultiPacketMessage(MessageType::SetPropertyData, common), request_id_(request_id), body_(body) {
    header_ = create_json_header(resource_identifier, res_id, "", set_partial, 0, 0);
}

std::vector<uint8_t> SetPropertyData::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32 + header_.size() + body_.size());
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(request_id_);
    
    uint16_t header_size = static_cast<uint16_t>(header_.size());
    data.push_back(static_cast<uint8_t>(header_size & 0x7F));
    data.push_back(static_cast<uint8_t>((header_size >> 7) & 0x7F));
    
    data.insert(data.end(), header_.begin(), header_.end());
    
    uint16_t body_size = static_cast<uint16_t>(body_.size());
    data.push_back(static_cast<uint8_t>(body_size & 0x7F));
    data.push_back(static_cast<uint8_t>((body_size >> 7) & 0x7F));
    
    data.insert(data.end(), body_.begin(), body_.end());
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool SetPropertyData::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 20;
}

std::string SetPropertyData::get_label() const {
    return "SetPropertyData";
}

std::string SetPropertyData::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", headerSize=" << header_.size() 
        << ", bodySize=" << body_.size();
    return oss.str();
}

SubscribeProperty::SubscribeProperty(const Common& common, uint8_t request_id, 
                                   const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : MultiPacketMessage(MessageType::SubscribeProperty, common), request_id_(request_id), header_(header), body_(body) {}

SubscribeProperty::SubscribeProperty(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                   const std::string& command, const std::string& mutual_encoding)
    : MultiPacketMessage(MessageType::SubscribeProperty, common), request_id_(request_id) {
    header_ = create_subscribe_json_header(resource_identifier, command, mutual_encoding);
}

std::vector<std::vector<uint8_t>> SubscribeProperty::serialize_multi() const {
    const uint32_t max_chunk_size = 256;
    
    if (header_.size() <= max_chunk_size) {
        return {serialize()};
    }
    
    std::vector<std::vector<uint8_t>> packets;
    size_t total_chunks = (header_.size() + max_chunk_size - 1) / max_chunk_size;
    
    for (size_t chunk_idx = 0; chunk_idx < total_chunks; ++chunk_idx) {
        size_t start_pos = chunk_idx * max_chunk_size;
        size_t chunk_size = std::min(static_cast<size_t>(max_chunk_size), header_.size() - start_pos);
        
        std::vector<uint8_t> chunk_data(header_.begin() + start_pos, header_.begin() + start_pos + chunk_size);
        
        std::vector<uint8_t> packet;
        packet.reserve(32 + chunk_data.size());
        
        packet.push_back(0x7E);
        packet.push_back(0x7F);
        packet.push_back(0x0D);
        packet.push_back(static_cast<uint8_t>(type_));
        packet.push_back(0x02);
        
        packet.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
        packet.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
        
        packet.push_back(request_id_);
        
        packet.push_back(static_cast<uint8_t>(total_chunks & 0x7F));
        packet.push_back(static_cast<uint8_t>((total_chunks >> 7) & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_idx + 1) & 0x7F));
        packet.push_back(static_cast<uint8_t>(((chunk_idx + 1) >> 7) & 0x7F));
        
        packet.push_back(static_cast<uint8_t>(chunk_data.size() & 0x7F));
        packet.push_back(static_cast<uint8_t>((chunk_data.size() >> 7) & 0x7F));
        
        packet.insert(packet.end(), chunk_data.begin(), chunk_data.end());
        
        packets.push_back(std::move(packet));
    }
    
    return packets;
}

std::vector<uint8_t> SubscribeProperty::create_subscribe_json_header(const std::string& resource_identifier, 
                                                                   const std::string& command, 
                                                                   const std::string& mutual_encoding) const {
    using namespace midi_ci::json;
    
    JsonValue header_json = JsonValue::empty_object();
    header_json["resource"] = JsonValue(resource_identifier);
    header_json["command"] = JsonValue(command);
    
    if (!mutual_encoding.empty()) {
        header_json["mutualEncoding"] = JsonValue(mutual_encoding);
    }
    
    return header_json.get_serialized_bytes();
}

std::vector<uint8_t> SubscribeProperty::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32 + header_.size() + body_.size());
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(request_id_);
    
    uint16_t header_size = static_cast<uint16_t>(header_.size());
    data.push_back(static_cast<uint8_t>(header_size & 0x7F));
    data.push_back(static_cast<uint8_t>((header_size >> 7) & 0x7F));
    
    data.insert(data.end(), header_.begin(), header_.end());
    
    uint16_t body_size = static_cast<uint16_t>(body_.size());
    data.push_back(static_cast<uint8_t>(body_size & 0x7F));
    data.push_back(static_cast<uint8_t>((body_size >> 7) & 0x7F));
    
    data.insert(data.end(), body_.begin(), body_.end());
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool SubscribeProperty::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 20;
}

std::string SubscribeProperty::get_label() const {
    return "SubscribeProperty";
}

std::string SubscribeProperty::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", headerSize=" << header_.size() 
        << ", bodySize=" << body_.size();
    return oss.str();
}

EndpointInquiry::EndpointInquiry(const Common& common, uint8_t status)
    : SinglePacketMessage(MessageType::EndpointInquiry, common), status_(status) {}

std::vector<uint8_t> EndpointInquiry::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(16);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(status_);
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool EndpointInquiry::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 16;
}

std::string EndpointInquiry::get_label() const {
    return "EndpointInquiry";
}

std::string EndpointInquiry::get_body_string() const {
    std::ostringstream oss;
    oss << "status=" << static_cast<int>(status_);
    return oss.str();
}

EndpointReply::EndpointReply(const Common& common, uint8_t status, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::EndpointReply, common), status_(status), data_(data) {
}

std::vector<uint8_t> EndpointReply::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> result;
    result.reserve(32);
    
    result.push_back(MIDI_CI_SYSEX_START);
    result.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    result.push_back(0x7F);
    result.push_back(MIDI_CI_SUB_ID_1);
    result.push_back(static_cast<uint8_t>(type_));
    result.push_back(MIDI_CI_VERSION_1_2);
    
    result.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    result.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    result.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    result.push_back(status_);
    result.insert(result.end(), data_.begin(), data_.end());
    
    result.push_back(MIDI_CI_SYSEX_END);
    return result;
}

bool EndpointReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 15) return false;
    
    status_ = data[14];
    if (data.size() > 16) {
        data_.assign(data.begin() + 15, data.end() - 1);
    } else {
        data_.clear();
    }
    return true;
}

std::string EndpointReply::get_label() const {
    return "EndpointReply";
}

std::string EndpointReply::get_body_string() const {
    std::ostringstream oss;
    oss << "status=" << static_cast<int>(status_);
    if (!data_.empty()) {
        oss << ", data_size=" << data_.size();
    }
    return oss.str();
}

InvalidateMUID::InvalidateMUID(const Common& common, uint32_t target_muid)
    : SinglePacketMessage(MessageType::InvalidateMUID, common), target_muid_(target_muid) {}

std::vector<uint8_t> InvalidateMUID::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(20);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(target_muid_ & 0x7F));
    data.push_back(static_cast<uint8_t>((target_muid_ >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((target_muid_ >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((target_muid_ >> 21) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool InvalidateMUID::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 19;
}

std::string InvalidateMUID::get_label() const {
    return "InvalidateMUID";
}

std::string InvalidateMUID::get_body_string() const {
    std::ostringstream oss;
    oss << "targetMUID=" << std::hex << target_muid_;
    return oss.str();
}

ProfileInquiry::ProfileInquiry(const Common& common)
    : SinglePacketMessage(MessageType::ProfileInquiry, common) {}

std::vector<uint8_t> ProfileInquiry::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(16);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProfileInquiry::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 15;
}

std::string ProfileInquiry::get_label() const {
    return "ProfileInquiry";
}

std::string ProfileInquiry::get_body_string() const {
    return "";
}

SetProfileOff::SetProfileOff(const Common& common, const std::vector<uint8_t>& profile_id)
    : SinglePacketMessage(MessageType::SetProfileOff, common), profile_id_(profile_id) {}

std::vector<uint8_t> SetProfileOff::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool SetProfileOff::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 20;
}

std::string SetProfileOff::get_label() const {
    return "SetProfileOff";
}

std::string SetProfileOff::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    return oss.str();
}

ProfileEnabledReport::ProfileEnabledReport(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabledReport::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(static_cast<uint8_t>(num_channels_ & 0x7F));
    data.push_back(static_cast<uint8_t>((num_channels_ >> 7) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProfileEnabledReport::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 22;
}

std::string ProfileEnabledReport::get_label() const {
    return "ProfileEnabledReport";
}

std::string ProfileEnabledReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

ProfileDisabledReport::ProfileDisabledReport(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabledReport::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(static_cast<uint8_t>(num_channels_ & 0x7F));
    data.push_back(static_cast<uint8_t>((num_channels_ >> 7) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProfileDisabledReport::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 22;
}

std::string ProfileDisabledReport::get_label() const {
    return "ProfileDisabledReport";
}

std::string ProfileDisabledReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

ProfileAddedReport::ProfileAddedReport(const Common& common, const std::vector<uint8_t>& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAddedReport::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProfileAddedReport::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 20;
}

std::string ProfileAddedReport::get_label() const {
    return "ProfileAddedReport";
}

std::string ProfileAddedReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    return oss.str();
}

ProfileRemovedReport::ProfileRemovedReport(const Common& common, const std::vector<uint8_t>& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemovedReport::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    for (size_t i = 0; i < midi_ci::core::constants::MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
        data.push_back(profile_id_[i]);
    }
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProfileRemovedReport::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 20;
}

std::string ProfileRemovedReport::get_label() const {
    return "ProfileRemovedReport";
}

std::string ProfileRemovedReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_[i]);
    }
    return oss.str();
}

MidiMessageReportInquiry::MidiMessageReportInquiry(const Common& common, uint8_t message_data_control,
                                                 uint8_t system_messages, uint8_t channel_controller_messages,
                                                 uint8_t note_data_messages)
    : SinglePacketMessage(MessageType::MidiMessageReportInquiry, common), 
      message_data_control_(message_data_control), system_messages_(system_messages),
      channel_controller_messages_(channel_controller_messages), note_data_messages_(note_data_messages) {}

std::vector<uint8_t> MidiMessageReportInquiry::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(20);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(message_data_control_);
    data.push_back(system_messages_);
    data.push_back(channel_controller_messages_);
    data.push_back(note_data_messages_);
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool MidiMessageReportInquiry::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 19;
}

std::string MidiMessageReportInquiry::get_label() const {
    return "MidiMessageReportInquiry";
}

std::string MidiMessageReportInquiry::get_body_string() const {
    std::ostringstream oss;
    oss << "messageDataControl=" << static_cast<int>(message_data_control_)
        << ", systemMessages=" << static_cast<int>(system_messages_)
        << ", channelControllerMessages=" << static_cast<int>(channel_controller_messages_)
        << ", noteDataMessages=" << static_cast<int>(note_data_messages_);
    return oss.str();
}

ProcessInquiryCapabilities::ProcessInquiryCapabilities(const Common& common)
    : SinglePacketMessage(MessageType::ProcessInquiryCapabilities, common) {}

std::vector<uint8_t> ProcessInquiryCapabilities::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(16);
    
    data.push_back(MIDI_CI_SYSEX_START);
    data.push_back(MIDI_CI_UNIVERSAL_SYSEX_ID);
    data.push_back(0x7F);
    data.push_back(MIDI_CI_SUB_ID_1);
    data.push_back(static_cast<uint8_t>(type_));
    data.push_back(MIDI_CI_VERSION_1_2);
    
    data.push_back(static_cast<uint8_t>(common_.source_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.source_muid >> 21) & 0x7F));
    
    data.push_back(static_cast<uint8_t>(common_.destination_muid & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 7) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 14) & 0x7F));
    data.push_back(static_cast<uint8_t>((common_.destination_muid >> 21) & 0x7F));
    
    data.push_back(MIDI_CI_SYSEX_END);
    
    return data;
}

bool ProcessInquiryCapabilities::deserialize(const std::vector<uint8_t>& data) {
    return data.size() >= 15;
}

std::string ProcessInquiryCapabilities::get_label() const {
    return "ProcessInquiryCapabilities";
}

std::string ProcessInquiryCapabilities::get_body_string() const {
    return "";
}

std::vector<std::vector<uint8_t>> Message::serialize_multi() const {
    return {serialize()};
}

ProfileReply::ProfileReply(const Common& common, const std::vector<std::vector<uint8_t>>& enabled_profiles, 
                          const std::vector<std::vector<uint8_t>>& disabled_profiles)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), enabled_profiles_(enabled_profiles), disabled_profiles_(disabled_profiles) {}

std::vector<uint8_t> ProfileReply::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(256);
    
    result.push_back(static_cast<uint8_t>(enabled_profiles_.size() & 0x7F));
    for (const auto& profile : enabled_profiles_) {
        result.insert(result.end(), profile.begin(), profile.end());
    }
    
    result.push_back(static_cast<uint8_t>(disabled_profiles_.size() & 0x7F));
    for (const auto& profile : disabled_profiles_) {
        result.insert(result.end(), profile.begin(), profile.end());
    }
    
    return result;
}

bool ProfileReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false;
    
    size_t pos = 0;
    uint8_t enabled_count = data[pos++] & 0x7F;
    
    enabled_profiles_.clear();
    for (int i = 0; i < enabled_count && pos + 5 <= data.size(); ++i) {
        std::vector<uint8_t> profile(data.begin() + pos, data.begin() + pos + 5);
        enabled_profiles_.push_back(profile);
        pos += 5;
    }
    
    if (pos >= data.size()) return false;
    uint8_t disabled_count = data[pos++] & 0x7F;
    
    disabled_profiles_.clear();
    for (int i = 0; i < disabled_count && pos + 5 <= data.size(); ++i) {
        std::vector<uint8_t> profile(data.begin() + pos, data.begin() + pos + 5);
        disabled_profiles_.push_back(profile);
        pos += 5;
    }
    
    return true;
}

std::string ProfileReply::get_label() const {
    return "ProfileReply";
}

std::string ProfileReply::get_body_string() const {
    return "enabled_profiles=" + std::to_string(enabled_profiles_.size()) + 
           ", disabled_profiles=" + std::to_string(disabled_profiles_.size());
}

PropertyGetCapabilitiesReply::PropertyGetCapabilitiesReply(const Common& common, uint8_t max_simultaneous_requests)
    : SinglePacketMessage(MessageType::PropertyGetCapabilitiesReply, common), max_simultaneous_requests_(max_simultaneous_requests) {}

std::vector<uint8_t> PropertyGetCapabilitiesReply::serialize() const {
    return {max_simultaneous_requests_};
}

bool PropertyGetCapabilitiesReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1) return false;
    max_simultaneous_requests_ = data[0];
    return true;
}

std::string PropertyGetCapabilitiesReply::get_label() const {
    return "PropertyGetCapabilitiesReply";
}

std::string PropertyGetCapabilitiesReply::get_body_string() const {
    return "max_simultaneous_requests=" + std::to_string(max_simultaneous_requests_);
}

GetPropertyDataReply::GetPropertyDataReply(const Common& common, uint8_t request_id, 
                                          const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : MultiPacketMessage(MessageType::GetPropertyDataReply, common), request_id_(request_id), header_(header), body_(body) {}

std::vector<uint8_t> GetPropertyDataReply::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(request_id_);
    result.insert(result.end(), header_.begin(), header_.end());
    result.insert(result.end(), body_.begin(), body_.end());
    return result;
}

std::vector<std::vector<uint8_t>> GetPropertyDataReply::serialize_multi() const {
    return {serialize()};
}

bool GetPropertyDataReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1) return false;
    request_id_ = data[0];
    
    if (data.size() > 1) {
        size_t header_end = data.size() / 2;
        header_.assign(data.begin() + 1, data.begin() + header_end);
        body_.assign(data.begin() + header_end, data.end());
    }
    
    return true;
}

std::string GetPropertyDataReply::get_label() const {
    return "GetPropertyDataReply";
}

std::string GetPropertyDataReply::get_body_string() const {
    return "request_id=" + std::to_string(request_id_) + 
           ", header_size=" + std::to_string(header_.size()) + 
           ", body_size=" + std::to_string(body_.size());
}

SetPropertyDataReply::SetPropertyDataReply(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header)
    : MultiPacketMessage(MessageType::SetPropertyDataReply, common), request_id_(request_id), header_(header) {}

std::vector<uint8_t> SetPropertyDataReply::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(request_id_);
    result.insert(result.end(), header_.begin(), header_.end());
    return result;
}

std::vector<std::vector<uint8_t>> SetPropertyDataReply::serialize_multi() const {
    return {serialize()};
}

bool SetPropertyDataReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1) return false;
    request_id_ = data[0];
    
    if (data.size() > 1) {
        header_.assign(data.begin() + 1, data.end());
    }
    
    return true;
}

std::string SetPropertyDataReply::get_label() const {
    return "SetPropertyDataReply";
}

std::string SetPropertyDataReply::get_body_string() const {
    return "request_id=" + std::to_string(request_id_) + 
           ", header_size=" + std::to_string(header_.size());
}

SubscribePropertyReply::SubscribePropertyReply(const Common& common, uint8_t request_id, 
                                              const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : MultiPacketMessage(MessageType::SubscribePropertyReply, common), request_id_(request_id), header_(header), body_(body) {}

std::vector<uint8_t> SubscribePropertyReply::serialize() const {
    std::vector<uint8_t> result;
    result.push_back(request_id_);
    result.insert(result.end(), header_.begin(), header_.end());
    result.insert(result.end(), body_.begin(), body_.end());
    return result;
}

std::vector<std::vector<uint8_t>> SubscribePropertyReply::serialize_multi() const {
    return {serialize()};
}

bool SubscribePropertyReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1) return false;
    request_id_ = data[0];
    
    if (data.size() > 1) {
        size_t header_end = data.size() / 2;
        header_.assign(data.begin() + 1, data.begin() + header_end);
        body_.assign(data.begin() + header_end, data.end());
    }
    
    return true;
}

std::string SubscribePropertyReply::get_label() const {
    return "SubscribePropertyReply";
}

std::string SubscribePropertyReply::get_body_string() const {
    return "request_id=" + std::to_string(request_id_) + 
           ", header_size=" + std::to_string(header_.size()) + 
           ", body_size=" + std::to_string(body_.size());
}

ProfileAdded::ProfileAdded(const Common& common, const std::vector<uint8_t>& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAdded::serialize() const {
    return profile_id_;
}

bool ProfileAdded::deserialize(const std::vector<uint8_t>& data) {
    profile_id_ = data;
    return true;
}

std::string ProfileAdded::get_label() const {
    return "ProfileAdded";
}

std::string ProfileAdded::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.size());
}

ProfileRemoved::ProfileRemoved(const Common& common, const std::vector<uint8_t>& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemoved::serialize() const {
    return profile_id_;
}

bool ProfileRemoved::deserialize(const std::vector<uint8_t>& data) {
    profile_id_ = data;
    return true;
}

std::string ProfileRemoved::get_label() const {
    return "ProfileRemoved";
}

std::string ProfileRemoved::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.size());
}

ProfileEnabled::ProfileEnabled(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabled::serialize() const {
    std::vector<uint8_t> result = profile_id_;
    result.push_back(static_cast<uint8_t>(num_channels_ & 0xFF));
    result.push_back(static_cast<uint8_t>((num_channels_ >> 8) & 0xFF));
    return result;
}

bool ProfileEnabled::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false;
    
    profile_id_.assign(data.begin(), data.end() - 2);
    num_channels_ = data[data.size() - 2] | (data[data.size() - 1] << 8);
    return true;
}

std::string ProfileEnabled::get_label() const {
    return "ProfileEnabled";
}

std::string ProfileEnabled::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.size()) + 
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDisabled::ProfileDisabled(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabled::serialize() const {
    std::vector<uint8_t> result = profile_id_;
    result.push_back(static_cast<uint8_t>(num_channels_ & 0xFF));
    result.push_back(static_cast<uint8_t>((num_channels_ >> 8) & 0xFF));
    return result;
}

bool ProfileDisabled::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 2) return false;
    
    profile_id_.assign(data.begin(), data.end() - 2);
    num_channels_ = data[data.size() - 2] | (data[data.size() - 1] << 8);
    return true;
}

std::string ProfileDisabled::get_label() const {
    return "ProfileDisabled";
}

std::string ProfileDisabled::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.size()) + 
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDetailsReply::ProfileDetailsReply(const Common& common, const std::vector<uint8_t>& profile_id, 
                                        uint8_t target, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), profile_id_(profile_id), target_(target), data_(data) {}

std::vector<uint8_t> ProfileDetailsReply::serialize() const {
    std::vector<uint8_t> result = profile_id_;
    result.push_back(target_);
    result.insert(result.end(), data_.begin(), data_.end());
    return result;
}

bool ProfileDetailsReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 6) return false;
    
    profile_id_.assign(data.begin(), data.begin() + 5);
    target_ = data[5];
    data_.assign(data.begin() + 6, data.end());
    return true;
}

std::string ProfileDetailsReply::get_label() const {
    return "ProfileDetailsReply";
}

std::string ProfileDetailsReply::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.size()) + 
           ", target=" + std::to_string(target_) + 
           ", data_size=" + std::to_string(data_.size());
}

ProcessInquiryCapabilitiesReply::ProcessInquiryCapabilitiesReply(const Common& common, uint8_t supported_features)
    : SinglePacketMessage(MessageType::ProcessInquiryCapabilitiesReply, common), supported_features_(supported_features) {}

std::vector<uint8_t> ProcessInquiryCapabilitiesReply::serialize() const {
    return {supported_features_};
}

bool ProcessInquiryCapabilitiesReply::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < 1) return false;
    supported_features_ = data[0];
    return true;
}

std::string ProcessInquiryCapabilitiesReply::get_label() const {
    return "ProcessInquiryCapabilitiesReply";
}

std::string ProcessInquiryCapabilitiesReply::get_body_string() const {
    return "supported_features=" + std::to_string(supported_features_);
}

} // namespace messages
} // namespace midi_ci
