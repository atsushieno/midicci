#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include "midi-ci/core/CIFactory.hpp"
#include "midi-ci/json/Json.hpp"
#include <sstream>
#include <iomanip>

namespace {
    std::string format_json_bytes(const std::vector<uint8_t>& bytes, size_t max_length = 4096) {
        if (bytes.empty()) {
            return "";
        }
        
        try {
            std::string json_str(bytes.begin(), bytes.end());
            if (json_str.length() > max_length) {
                json_str = json_str.substr(0, max_length);
            }
            
            auto json_value = midi_ci::json::JsonValue::parse(json_str);
            return json_value.serialize();
        } catch (...) {
            std::string fallback(bytes.begin(), bytes.end());
            if (fallback.length() > max_length) {
                fallback = fallback.substr(0, max_length);
            }
            return fallback;
        }
    }
}

using namespace midi_ci::core::constants;

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

DiscoveryInquiry::DiscoveryInquiry(const Common& common, const core::DeviceDetails& device_details,
                                 uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id)
    : SinglePacketMessage(MessageType::DiscoveryInquiry, common), device_details_(device_details),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), output_path_id_(output_path_id) {}

std::vector<uint8_t> DiscoveryInquiry::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(64);
    return core::CIFactory::midiCIDiscovery(data, common_.source_muid,
                                     device_details_.manufacturer,
                                     device_details_.family,
                                     device_details_.modelNumber,
                                     device_details_.softwareRevisionLevel,
                                     supported_features_, max_sysex_size_, output_path_id_);
}

std::string DiscoveryInquiry::get_label() const {
    return "DiscoveryInquiry";
}

std::string DiscoveryInquiry::get_body_string() const {
    std::ostringstream oss;
    oss << "manufacturer=" << device_details_.manufacturer
        << ", family=" << device_details_.family
        << ", modelNumber=" << device_details_.modelNumber
        << ", softwareRevisionLevel=" << device_details_.softwareRevisionLevel
        << ", features=" << std::hex << static_cast<int>(supported_features_)
        << ", maxSysEx=" << max_sysex_size_
        << ", outputPath=" << static_cast<int>(output_path_id_);
    return oss.str();
}

DiscoveryReply::DiscoveryReply(const Common& common, const core::DeviceDetails& device_details,
                              uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id, uint8_t function_block)
    : SinglePacketMessage(MessageType::DiscoveryReply, common), device_details_(device_details),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), 
      output_path_id_(output_path_id), function_block_(function_block) {}

std::vector<uint8_t> DiscoveryReply::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(64);

    return core::CIFactory::midiCIDiscoveryReply(data, common_.address,
                                                 common_.source_muid, common_.destination_muid,
                                                 device_details_.manufacturer,
                                                 device_details_.family,
                                                 device_details_.modelNumber,
                                                 device_details_.softwareRevisionLevel,
                                                 supported_features_, max_sysex_size_,
                                                 output_path_id_, function_block_);
}

std::string DiscoveryReply::get_label() const {
    return "DiscoveryReply";
}

std::string DiscoveryReply::get_body_string() const {
    std::ostringstream oss;
    oss << "manufacturer=" << device_details_.manufacturer
        << ", family=" << device_details_.family
        << ", modelNumber=" << device_details_.modelNumber
        << ", softwareRevisionLevel=" << device_details_.softwareRevisionLevel
        << ", features=" << std::hex << static_cast<int>(supported_features_)
        << ", maxSysEx=" << max_sysex_size_
        << ", outputPath=" << static_cast<int>(output_path_id_)
        << ", functionBlock=" << static_cast<int>(function_block_);
    return oss.str();
}

SetProfileOn::SetProfileOn(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::SetProfileOn, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> SetProfileOn::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileSet(data, common_.address, true, 
                                             common_.source_muid, common_.destination_muid, 
                                             profile_id, num_channels_);
}

std::string SetProfileOn::get_label() const {
    return "SetProfileOn";
}

std::string SetProfileOn::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=";
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

PropertyGetCapabilities::PropertyGetCapabilities(const Common& common, uint8_t max_simultaneous_requests)
    : SinglePacketMessage(MessageType::PropertyGetCapabilities, common), max_simultaneous_requests_(max_simultaneous_requests) {}

std::vector<uint8_t> PropertyGetCapabilities::serialize() const {
    std::vector<uint8_t> data;
    data.reserve(16);

    return core::CIFactory::midiCIPropertyGetCapabilities(data, common_.address, false,
                                                          common_.source_muid, common_.destination_muid,
                                                          max_simultaneous_requests_);
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
    
    auto json_str = header_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<std::vector<uint8_t>> GetPropertyData::serialize_multi() const {
    std::vector<uint8_t> dst(4096);
    return midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_GET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, {});
}

std::vector<std::vector<uint8_t>> SetPropertyData::serialize_multi() const {
    std::vector<uint8_t> dst(4096);
    return midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
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
    
    auto json_str = header_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> GetPropertyData::serialize() const {
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_GET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, {});
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}



std::string GetPropertyData::get_label() const {
    return "GetPropertyData";
}

std::string GetPropertyData::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=";
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
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}



std::string SetPropertyData::get_label() const {
    return "SetPropertyData";
}

std::string SetPropertyData::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
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
    std::vector<uint8_t> dst(4096);
    return midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
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
    
    auto json_str = header_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> SubscribeProperty::serialize() const {
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}

std::string SubscribeProperty::get_label() const {
    return "SubscribeProperty";
}

std::string SubscribeProperty::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

EndpointInquiry::EndpointInquiry(const Common& common, uint8_t status)
    : SinglePacketMessage(MessageType::EndpointInquiry, common), status_(status) {}

std::vector<uint8_t> EndpointInquiry::serialize() const {
    std::vector<uint8_t> data;
    data.resize(16);
    
    return core::CIFactory::midiCIEndpointMessage(data, MIDI_CI_VERSION_1_2, 
                                                 common_.source_muid, common_.destination_muid, 
                                                 status_);
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
    std::vector<uint8_t> result;
    result.resize(32 + data_.size());
    
    return core::CIFactory::midiCIEndpointMessageReply(result, MIDI_CI_VERSION_1_2, 
                                                      common_.source_muid, common_.destination_muid, 
                                                      status_, data_);
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
    std::vector<uint8_t> data;
    data.resize(32);
    
    return core::CIFactory::midiCIInvalidateMuid(data, common_.address, 
                                                common_.source_muid, target_muid_);
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
    std::vector<uint8_t> data;
    data.reserve(16);
    return core::CIFactory::midiCIProfileInquiry(data, common_.address, common_.source_muid, common_.destination_muid);
}

std::string ProfileInquiry::get_label() const {
    return "ProfileInquiry";
}

std::string ProfileInquiry::get_body_string() const {
    return "";
}

SetProfileOff::SetProfileOff(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::SetProfileOff, common), profile_id_(profile_id) {}

std::vector<uint8_t> SetProfileOff::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileSet(data, common_.address, false, 
                                             common_.source_muid, common_.destination_muid, 
                                             profile_id, 0);
}

std::string SetProfileOff::get_label() const {
    return "SetProfileOff";
}

std::string SetProfileOff::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    return oss.str();
}

ProfileEnabledReport::ProfileEnabledReport(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabledReport::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileReport(data, common_.address, true, 
                                                common_.source_muid, profile_id, num_channels_);
}

std::string ProfileEnabledReport::get_label() const {
    return "ProfileEnabledReport";
}

std::string ProfileEnabledReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    oss << ", numChannels=" << std::dec << num_channels_;
    return oss.str();
}

ProfileDisabledReport::ProfileDisabledReport(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabledReport::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileReport(data, common_.address, false, 
                                                common_.source_muid, profile_id, num_channels_);
}

std::string ProfileDisabledReport::get_label() const {
    return "ProfileDisabledReport";
}

std::string ProfileDisabledReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

ProfileAddedReport::ProfileAddedReport(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAddedReport::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileAddedRemoved(data, common_.address, false, 
                                                     common_.source_muid, profile_id);
}

std::string ProfileAddedReport::get_label() const {
    return "ProfileAddedReport";
}

std::string ProfileAddedReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    return oss.str();
}

ProfileRemovedReport::ProfileRemovedReport(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemovedReport::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    midi_ci::profiles::MidiCIProfileId profile_id(profile_id_);
    return core::CIFactory::midiCIProfileAddedRemoved(data, common_.address, true, 
                                                     common_.source_muid, profile_id);
}

std::string ProfileRemovedReport::get_label() const {
    return "ProfileRemovedReport";
}

std::string ProfileRemovedReport::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
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
    std::vector<uint8_t> data;
    data.resize(32);
    
    return core::CIFactory::midiCIMidiMessageReport(data, common_.address, 
                                                   common_.source_muid, common_.destination_muid,
                                                   message_data_control_, system_messages_, 
                                                   channel_controller_messages_, note_data_messages_);
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
    std::vector<uint8_t> data;
    data.reserve(16);
    return core::CIFactory::midiCIProcessInquiryCapabilities(data, common_.source_muid, common_.destination_muid);
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

std::string Message::get_log_message() const {
    std::ostringstream oss;
    oss << get_label() << ": " << get_body_string();
    return oss.str();
}

ProfileReply::ProfileReply(const Common& common, const std::vector<profiles::MidiCIProfileId>& enabled_profiles,
                          const std::vector<profiles::MidiCIProfileId>& disabled_profiles)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), enabled_profiles_(enabled_profiles), disabled_profiles_(disabled_profiles) {}

std::vector<uint8_t> ProfileReply::serialize() const {
    std::vector<uint8_t> result;
    result.reserve(256);
    return core::CIFactory::midiCIProfileInquiryReply(result, common_.address,
                                               common_.source_muid, common_.destination_muid,
                                               enabled_profiles_, disabled_profiles_);
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
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_GET_DATA_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}

std::vector<std::vector<uint8_t>> GetPropertyDataReply::serialize_multi() const {
    return {serialize()};
}



std::string GetPropertyDataReply::get_label() const {
    return "GetPropertyDataReply";
}

std::string GetPropertyDataReply::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

SetPropertyDataReply::SetPropertyDataReply(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header)
    : MultiPacketMessage(MessageType::SetPropertyDataReply, common), request_id_(request_id), header_(header) {}

std::vector<uint8_t> SetPropertyDataReply::serialize() const {
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SET_DATA_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, {});
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}

std::vector<std::vector<uint8_t>> SetPropertyDataReply::serialize_multi() const {
    return {serialize()};
}



std::string SetPropertyDataReply::get_label() const {
    return "SetPropertyDataReply";
}

std::string SetPropertyDataReply::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=";
    return oss.str();
}

SubscribePropertyReply::SubscribePropertyReply(const Common& common, uint8_t request_id, 
                                              const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : MultiPacketMessage(MessageType::SubscribePropertyReply, common), request_id_(request_id), header_(header), body_(body) {}

std::vector<uint8_t> SubscribePropertyReply::serialize() const {
    std::vector<uint8_t> dst(4096);
    auto chunks = midi_ci::core::CIFactory::midiCIPropertyChunks(
        dst, 4096 - 256, // max chunk size
        static_cast<uint8_t>(midi_ci::core::constants::CISubId2::PROPERTY_SUBSCRIPTION_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks.empty() ? std::vector<uint8_t>() : chunks[0];
}

std::vector<std::vector<uint8_t>> SubscribePropertyReply::serialize_multi() const {
    return {serialize()};
}

std::string SubscribePropertyReply::get_label() const {
    return "SubscribePropertyReply";
}

std::string SubscribePropertyReply::get_body_string() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

ProfileAdded::ProfileAdded(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAdded::serialize() const {
    std::vector<uint8_t> dst(256);
    return midi_ci::core::CIFactory::midiCIProfileAddedRemoved(dst, common_.address, false, common_.source_muid, profile_id_);
}

std::string ProfileAdded::get_label() const {
    return "ProfileAdded";
}

std::string ProfileAdded::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size());
}

ProfileRemoved::ProfileRemoved(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemoved::serialize() const {
    std::vector<uint8_t> dst(256);
    return midi_ci::core::CIFactory::midiCIProfileAddedRemoved(dst, common_.address, true, common_.source_muid, profile_id_);
}

std::string ProfileRemoved::get_label() const {
    return "ProfileRemoved";
}

std::string ProfileRemoved::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size());
}

ProfileEnabled::ProfileEnabled(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabled::serialize() const {
    std::vector<uint8_t> dst(256);
    return midi_ci::core::CIFactory::midiCIProfileReport(dst, common_.address, true, common_.source_muid, profile_id_, num_channels_);
}

std::string ProfileEnabled::get_label() const {
    return "ProfileEnabled";
}

std::string ProfileEnabled::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDisabled::ProfileDisabled(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabled::serialize() const {
    std::vector<uint8_t> dst(256);
    return midi_ci::core::CIFactory::midiCIProfileReport(dst, common_.address, false, common_.source_muid, profile_id_, num_channels_);
}

std::string ProfileDisabled::get_label() const {
    return "ProfileDisabled";
}

std::string ProfileDisabled::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDetailsReply::ProfileDetailsReply(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id,
                                        uint8_t target, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), profile_id_(profile_id), target_(target), data_(data) {}

std::vector<uint8_t> ProfileDetailsReply::serialize() const {
    std::vector<uint8_t> dst(256);
    return midi_ci::core::CIFactory::midiCIProfileDetailsReply(dst, common_.address, false, common_.source_muid, profile_id_, target_, data_);
}

std::string ProfileDetailsReply::get_label() const {
    return "ProfileDetailsReply";
}

std::string ProfileDetailsReply::get_body_string() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", target=" + std::to_string(target_) + 
           ", data_size=" + std::to_string(data_.size());
}

ProcessInquiryCapabilitiesReply::ProcessInquiryCapabilitiesReply(const Common& common, uint8_t supported_features)
    : SinglePacketMessage(MessageType::ProcessInquiryCapabilitiesReply, common), supported_features_(supported_features) {}

std::vector<uint8_t> ProcessInquiryCapabilitiesReply::serialize() const {
    return {supported_features_};
}

std::string ProcessInquiryCapabilitiesReply::get_label() const {
    return "ProcessInquiryCapabilitiesReply";
}

std::string ProcessInquiryCapabilitiesReply::get_body_string() const {
    return "supported_features=" + std::to_string(supported_features_);
}

// MidiMessageReportReply implementation
MidiMessageReportReply::MidiMessageReportReply(const Common& common, uint8_t system_messages, uint8_t channel_controller_messages, uint8_t note_data_messages)
    : SinglePacketMessage(MessageType::MidiMessageReportInquiry, common), system_messages_(system_messages), channel_controller_messages_(channel_controller_messages), note_data_messages_(note_data_messages) {}

std::vector<uint8_t> MidiMessageReportReply::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return core::CIFactory::midiCIMidiMessageReportReply(data, common_.address, 
                                                        common_.source_muid, common_.destination_muid,
                                                        system_messages_, channel_controller_messages_, 
                                                        note_data_messages_);
}

std::string MidiMessageReportReply::get_label() const {
    return "MidiMessageReportReply";
}

std::string MidiMessageReportReply::get_body_string() const {
    std::ostringstream oss;
    oss << "systemMessages=" << static_cast<int>(system_messages_) 
        << ", channelControllerMessages=" << static_cast<int>(channel_controller_messages_)
        << ", noteDataMessages=" << static_cast<int>(note_data_messages_);
    return oss.str();
}

// MidiMessageReportNotifyEnd implementation
MidiMessageReportNotifyEnd::MidiMessageReportNotifyEnd(const Common& common)
    : SinglePacketMessage(MessageType::MidiMessageReportInquiry, common) {}

std::vector<uint8_t> MidiMessageReportNotifyEnd::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return core::CIFactory::midiCIEndOfMidiMessage(data, common_.address, 
                                                  common_.source_muid, common_.destination_muid);
}

std::string MidiMessageReportNotifyEnd::get_label() const {
    return "MidiMessageReportNotifyEnd";
}

std::string MidiMessageReportNotifyEnd::get_body_string() const {
    return "";
}

ProfileSpecificData::ProfileSpecificData(const Common& common, const midi_ci::profiles::MidiCIProfileId& profile_id, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::ProfileInquiry, common), profile_id_(profile_id), data_(data) {}

std::vector<uint8_t> ProfileSpecificData::serialize() const {
    std::vector<uint8_t> data;
    data.resize(32 + data_.size());
    return core::CIFactory::midiCIProfileSpecificData(data, common_.address,
                                                      common_.source_muid, common_.destination_muid,
                                                      profile_id_, data_);
}

std::string ProfileSpecificData::get_label() const {
    return "ProfileSpecificData";
}

std::string ProfileSpecificData::get_body_string() const {
    std::ostringstream oss;
    oss << "profileId=[" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "0x" << static_cast<int>(profile_id_.data[i]);
    }
    oss << "], dataSize=" << std::dec << data_.size();
    return oss.str();
}

} // namespace messages
} // namespace midi_ci
