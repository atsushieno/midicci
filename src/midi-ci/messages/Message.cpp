#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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

std::vector<uint8_t> GetPropertyData::serialize() const {
    using namespace midi_ci::core::constants;
    
    std::vector<uint8_t> data;
    data.reserve(32 + header_.size());
    
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
    
    data.push_back(MIDI_CI_SYSEX_END);
    
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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
    
    for (size_t i = 0; i < MIDI_CI_PROFILE_ID_SIZE && i < profile_id_.size(); ++i) {
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

} // namespace messages
} // namespace midi_ci
