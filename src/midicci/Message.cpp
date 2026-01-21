#include "midicci/midicci.hpp"
#include <sstream>
#include <iomanip>

namespace midicci {

    std::string format_json_bytes(const std::vector<uint8_t>& bytes, size_t max_length = 4096) {
        if (bytes.empty()) {
            return "";
        }
        
        try {
            std::string json_str(bytes.begin(), bytes.end());
            if (json_str.length() > max_length) {
                json_str = json_str.substr(0, max_length);
            }
            
            auto json_value = midicci::JsonValue::parse(json_str);
            return json_value.serialize();
        } catch (...) {
            std::string fallback(bytes.begin(), bytes.end());
            if (fallback.length() > max_length) {
                fallback = fallback.substr(0, max_length);
            }
            return fallback;
        }
    }

Message::Message(MessageType type, const Common& common)
    : type_(type), common_(common) {}

std::vector<std::vector<uint8_t>> Message::serializeMulti(const MidiCIDeviceConfiguration& config) const {
    return {};
}

MessageType Message::getType() const noexcept {
    return type_;
}

uint32_t Message::getSourceMuid() const noexcept {
    return common_.source_muid;
}

uint32_t Message::getDestinationMuid() const noexcept {
    return common_.destination_muid;
}

SinglePacketMessage::SinglePacketMessage(MessageType type, const Common& common)
    : Message(type, common) {}

std::vector<std::vector<uint8_t>> SinglePacketMessage::serializeMulti(const MidiCIDeviceConfiguration& config) const {
    auto single = serialize(config);
    return {single};
}

PropertyMessage::PropertyMessage(MessageType type, const Common& common, uint8_t request_id,
                               const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : Message(type, common), request_id_(request_id), header_(header), body_(body) {}

std::vector<std::vector<uint8_t>> PropertyMessage::serializeMulti(const MidiCIDeviceConfiguration& config) const {
    return serialize(config);
}

DiscoveryInquiry::DiscoveryInquiry(const Common& common, const DeviceDetails& device_details,
                                 uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id)
    : SinglePacketMessage(MessageType::DiscoveryInquiry, common), device_details_(device_details),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), output_path_id_(output_path_id) {}

std::vector<uint8_t> DiscoveryInquiry::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.reserve(64);
    return CIFactory::midiCIDiscovery(data, common_.source_muid,
                                     device_details_.manufacturer,
                                     device_details_.family,
                                     device_details_.modelNumber,
                                     device_details_.softwareRevisionLevel,
                                     supported_features_, max_sysex_size_, output_path_id_);
}

std::string DiscoveryInquiry::getLabel() const {
    return "DiscoveryInquiry";
}

std::string DiscoveryInquiry::getBodyString() const {
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

DiscoveryReply::DiscoveryReply(const Common& common, const DeviceDetails& device_details,
                              uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id, uint8_t function_block)
    : SinglePacketMessage(MessageType::DiscoveryReply, common), device_details_(device_details),
      supported_features_(supported_features), max_sysex_size_(max_sysex_size), 
      output_path_id_(output_path_id), function_block_(function_block) {}

std::vector<uint8_t> DiscoveryReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.reserve(64);

    return CIFactory::midiCIDiscoveryReply(data, common_.address,
                                                 common_.source_muid, common_.destination_muid,
                                                 device_details_.manufacturer,
                                                 device_details_.family,
                                                 device_details_.modelNumber,
                                                 device_details_.softwareRevisionLevel,
                                                 supported_features_, max_sysex_size_,
                                                 output_path_id_, function_block_);
}

std::string DiscoveryReply::getLabel() const {
    return "DiscoveryReply";
}

std::string DiscoveryReply::getBodyString() const {
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

SetProfileOn::SetProfileOn(const Common& common, const MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::SetProfileOn, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> SetProfileOn::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileSet(data, common_.address, true, 
                                             common_.source_muid, common_.destination_muid, 
                                             profile_id, num_channels_);
}

std::string SetProfileOn::getLabel() const {
    return "SetProfileOn";
}

std::string SetProfileOn::getBodyString() const {
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

std::vector<uint8_t> PropertyGetCapabilities::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.reserve(16);

    return CIFactory::midiCIPropertyGetCapabilities(data, common_.address, false,
                                                          common_.source_muid, common_.destination_muid,
                                                          max_simultaneous_requests_);
}

std::string PropertyGetCapabilities::getLabel() const {
    return "PropertyGetCapabilities";
}

std::string PropertyGetCapabilities::getBodyString() const {
    std::ostringstream oss;
    oss << "maxSimultaneousRequests=" << static_cast<int>(max_simultaneous_requests_);
    return oss.str();
}

GetPropertyData::GetPropertyData(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header)
    : PropertyMessage(MessageType::GetPropertyData, common, request_id, header, {}) {}

GetPropertyData::GetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                const std::string& res_id)
    : PropertyMessage(MessageType::GetPropertyData, common, request_id, {}, {}) {
    header_ = createJsonHeader(resource_identifier, res_id, "", false, 0, 0);
}

std::vector<uint8_t> GetPropertyData::createJsonHeader(const std::string& resource_identifier, 
                                                        const std::string& res_id, 
                                                        const std::string& mutual_encoding,
                                                        bool set_partial,
                                                        int offset, 
                                                        int limit) const {
    JsonValue header_json = JsonValue::emptyObject();
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



std::vector<std::vector<uint8_t>> GetPropertyData::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(config.receivable_max_sysex_size);
    return CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_GET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, {});
}



std::vector<std::vector<uint8_t>> SetPropertyData::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(config.receivable_max_sysex_size);
    return CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_SET_DATA_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
}

std::vector<uint8_t> SetPropertyData::createJsonHeader(const std::string& resource_identifier, const std::string& res_id, 
                                                        const std::string& mutual_encoding, bool set_partial, 
                                                        int offset, int limit) const {
    JsonValue header_json = JsonValue::emptyObject();
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





std::string GetPropertyData::getLabel() const {
    return "GetPropertyData";
}

std::string GetPropertyData::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=";
    return oss.str();
}

SetPropertyData::SetPropertyData(const Common& common, uint8_t request_id, 
                                 const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : PropertyMessage(MessageType::SetPropertyData, common, request_id, header, body) {}

SetPropertyData::SetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                 const std::vector<uint8_t>& body, const std::string& res_id, bool set_partial)
    : PropertyMessage(MessageType::SetPropertyData, common, request_id, {}, body) {
    header_ = createJsonHeader(resource_identifier, res_id, "", set_partial, 0, 0);
}





std::string SetPropertyData::getLabel() const {
    return "SetPropertyData";
}

std::string SetPropertyData::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

SubscribeProperty::SubscribeProperty(const Common& common, uint8_t request_id, 
                                   const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : PropertyMessage(MessageType::SubscribeProperty, common, request_id, header, body) {}

SubscribeProperty::SubscribeProperty(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                                   const std::string& command, const std::string& mutual_encoding)
    : PropertyMessage(MessageType::SubscribeProperty, common, request_id, {}, {}) {
    header_ = createSubscribeJsonHeader(resource_identifier, command, mutual_encoding);
}



std::vector<std::vector<uint8_t>> SubscribeProperty::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(config.receivable_max_sysex_size);
    return CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
}

std::vector<uint8_t> SubscribeProperty::createSubscribeJsonHeader(const std::string& resource_identifier, 
                                                                   const std::string& command, 
                                                                   const std::string& mutual_encoding) const {
    JsonValue header_json = JsonValue::emptyObject();
    header_json["resource"] = JsonValue(resource_identifier);
    header_json["command"] = JsonValue(command);
    
    if (!mutual_encoding.empty()) {
        header_json["mutualEncoding"] = JsonValue(mutual_encoding);
    }
    
    auto json_str = header_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}



std::string SubscribeProperty::getLabel() const {
    return "SubscribeProperty";
}

std::string SubscribeProperty::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

EndpointInquiry::EndpointInquiry(const Common& common, uint8_t status)
    : SinglePacketMessage(MessageType::EndpointInquiry, common), status_(status) {}

std::vector<uint8_t> EndpointInquiry::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(16);
    
    return CIFactory::midiCIEndpointMessage(data, MIDI_CI_VERSION_1_2, 
                                                 common_.source_muid, common_.destination_muid, 
                                                 status_);
}

std::string EndpointInquiry::getLabel() const {
    return "EndpointInquiry";
}

std::string EndpointInquiry::getBodyString() const {
    std::ostringstream oss;
    oss << "status=" << static_cast<int>(status_);
    return oss.str();
}

EndpointReply::EndpointReply(const Common& common, uint8_t status, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::EndpointReply, common), status_(status), data_(data) {
}

std::vector<uint8_t> EndpointReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> result;
    result.resize(32 + data_.size());
    
    return CIFactory::midiCIEndpointMessageReply(result, MIDI_CI_VERSION_1_2, 
                                                      common_.source_muid, common_.destination_muid, 
                                                      status_, data_);
}

std::string EndpointReply::getLabel() const {
    return "EndpointReply";
}

std::string EndpointReply::getBodyString() const {
    std::ostringstream oss;
    oss << "status=" << static_cast<int>(status_);
    if (!data_.empty()) {
        oss << ", data_size=" << data_.size();
    }
    return oss.str();
}

InvalidateMUID::InvalidateMUID(const Common& common, uint32_t target_muid)
    : SinglePacketMessage(MessageType::InvalidateMUID, common), target_muid_(target_muid) {}

std::vector<uint8_t> InvalidateMUID::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return CIFactory::midiCIInvalidateMuid(data, common_.address, 
                                                common_.source_muid, target_muid_);
}

std::string InvalidateMUID::getLabel() const {
    return "InvalidateMUID";
}

std::string InvalidateMUID::getBodyString() const {
    std::ostringstream oss;
    oss << "targetMUID=" << std::hex << target_muid_;
    return oss.str();
}

ProfileInquiry::ProfileInquiry(const Common& common)
    : SinglePacketMessage(MessageType::ProfileInquiry, common) {}

std::vector<uint8_t> ProfileInquiry::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.reserve(16);
    return CIFactory::midiCIProfileInquiry(data, common_.address, common_.source_muid, common_.destination_muid);
}

std::string ProfileInquiry::getLabel() const {
    return "ProfileInquiry";
}

std::string ProfileInquiry::getBodyString() const {
    return "";
}

SetProfileOff::SetProfileOff(const Common& common, const MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::SetProfileOff, common), profile_id_(profile_id) {}

std::vector<uint8_t> SetProfileOff::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileSet(data, common_.address, false, 
                                             common_.source_muid, common_.destination_muid, 
                                             profile_id, 0);
}

std::string SetProfileOff::getLabel() const {
    return "SetProfileOff";
}

std::string SetProfileOff::getBodyString() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    return oss.str();
}

ProfileEnabledReport::ProfileEnabledReport(const Common& common, const MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabledReport::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileReport(data, common_.address, true, 
                                                common_.source_muid, profile_id, num_channels_);
}

std::string ProfileEnabledReport::getLabel() const {
    return "ProfileEnabledReport";
}

std::string ProfileEnabledReport::getBodyString() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    oss << ", numChannels=" << std::dec << num_channels_;
    return oss.str();
}

ProfileDisabledReport::ProfileDisabledReport(const Common& common, const MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabledReport::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileReport(data, common_.address, false, 
                                                common_.source_muid, profile_id, num_channels_);
}

std::string ProfileDisabledReport::getLabel() const {
    return "ProfileDisabledReport";
}

std::string ProfileDisabledReport::getBodyString() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    oss << ", numChannels=" << num_channels_;
    return oss.str();
}

ProfileAddedReport::ProfileAddedReport(const Common& common, const MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAddedReport::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileAddedRemoved(data, common_.address, false, 
                                                     common_.source_muid, profile_id);
}

std::string ProfileAddedReport::getLabel() const {
    return "ProfileAddedReport";
}

std::string ProfileAddedReport::getBodyString() const {
    std::ostringstream oss;
    oss << "profileId=" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ":";
        oss << std::hex << static_cast<int>(profile_id_.data[i]);
    }
    return oss.str();
}

ProfileRemovedReport::ProfileRemovedReport(const Common& common, const MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemovedReport::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    MidiCIProfileId profile_id(profile_id_);
    return CIFactory::midiCIProfileAddedRemoved(data, common_.address, true, 
                                                     common_.source_muid, profile_id);
}

std::string ProfileRemovedReport::getLabel() const {
    return "ProfileRemovedReport";
}

std::string ProfileRemovedReport::getBodyString() const {
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

std::vector<uint8_t> MidiMessageReportInquiry::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return CIFactory::midiCIMidiMessageReport(data, common_.address, 
                                                   common_.source_muid, common_.destination_muid,
                                                   message_data_control_, system_messages_, 
                                                   channel_controller_messages_, note_data_messages_);
}

std::string MidiMessageReportInquiry::getLabel() const {
    return "MidiMessageReportInquiry";
}

std::string MidiMessageReportInquiry::getBodyString() const {
    std::ostringstream oss;
    oss << "messageDataControl=" << static_cast<int>(message_data_control_)
        << ", systemMessages=" << static_cast<int>(system_messages_)
        << ", channelControllerMessages=" << static_cast<int>(channel_controller_messages_)
        << ", noteDataMessages=" << static_cast<int>(note_data_messages_);
    return oss.str();
}

ProcessInquiryCapabilities::ProcessInquiryCapabilities(const Common& common)
    : SinglePacketMessage(MessageType::ProcessInquiryCapabilities, common) {}

std::vector<uint8_t> ProcessInquiryCapabilities::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.reserve(16);
    return CIFactory::midiCIProcessInquiryCapabilities(data, common_.source_muid, common_.destination_muid);
}

std::string ProcessInquiryCapabilities::getLabel() const {
    return "ProcessInquiryCapabilities";
}

std::string ProcessInquiryCapabilities::getBodyString() const {
    return "";
}



std::string Message::getLogMessage() const {
    std::ostringstream oss;
    oss << getLabel() << ": " << getBodyString();
    return oss.str();
}

ProfileReply::ProfileReply(const Common& common, const std::vector<MidiCIProfileId>& enabled_profiles,
                          const std::vector<MidiCIProfileId>& disabled_profiles)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), enabled_profiles_(enabled_profiles), disabled_profiles_(disabled_profiles) {}

std::vector<uint8_t> ProfileReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> result;
    result.reserve(256);
    return CIFactory::midiCIProfileInquiryReply(result, common_.address,
                                               common_.source_muid, common_.destination_muid,
                                               enabled_profiles_, disabled_profiles_);
}



std::string ProfileReply::getLabel() const {
    return "ProfileReply";
}

std::string ProfileReply::getBodyString() const {
    return "enabled_profiles=" + std::to_string(enabled_profiles_.size()) + 
           ", disabled_profiles=" + std::to_string(disabled_profiles_.size());
}

PropertyGetCapabilitiesReply::PropertyGetCapabilitiesReply(const Common& common, uint8_t max_simultaneous_requests)
    : SinglePacketMessage(MessageType::PropertyGetCapabilitiesReply, common), max_simultaneous_requests_(max_simultaneous_requests) {}

std::vector<uint8_t> PropertyGetCapabilitiesReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    return CIFactory::midiCIPropertyExchangeCapabilitiesReply(data, common_.address,
                                                      common_.source_muid, common_.destination_muid, max_simultaneous_requests_);
}

std::string PropertyGetCapabilitiesReply::getLabel() const {
    return "PropertyGetCapabilitiesReply";
}

std::string PropertyGetCapabilitiesReply::getBodyString() const {
    return "max_simultaneous_requests=" + std::to_string(max_simultaneous_requests_);
}

GetPropertyDataReply::GetPropertyDataReply(const Common& common, uint8_t request_id, 
                                          const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : PropertyMessage(MessageType::GetPropertyDataReply, common, request_id, header, body) {}

std::vector<std::vector<uint8_t>> GetPropertyDataReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(config.receivable_max_sysex_size);
    auto chunks = CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_GET_DATA_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks;
}





std::string GetPropertyDataReply::getLabel() const {
    return "GetPropertyDataReply";
}

std::string GetPropertyDataReply::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

SetPropertyDataReply::SetPropertyDataReply(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header)
    : PropertyMessage(MessageType::SetPropertyDataReply, common, request_id, header, {}) {}

std::vector<std::vector<uint8_t>> SetPropertyDataReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(4096);
    auto chunks = CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_SET_DATA_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, {});
    return chunks;
}





std::string SetPropertyDataReply::getLabel() const {
    return "SetPropertyDataReply";
}

std::string SetPropertyDataReply::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=";
    return oss.str();
}

SubscribePropertyReply::SubscribePropertyReply(const Common& common, uint8_t request_id, 
                                              const std::vector<uint8_t>& header, const std::vector<uint8_t>& body)
    : PropertyMessage(MessageType::SubscribePropertyReply, common, request_id, header, body) {}

std::vector<std::vector<uint8_t>> SubscribePropertyReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(4096);
    auto chunks = CIFactory::midiCIPropertyChunks(
        dst, config.max_property_chunk_size,
        static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_REPLY),
        common_.source_muid, common_.destination_muid, request_id_, header_, body_);
    return chunks;
}



std::string SubscribePropertyReply::getLabel() const {
    return "SubscribePropertyReply";
}

std::string SubscribePropertyReply::getBodyString() const {
    std::ostringstream oss;
    oss << "requestId=" << static_cast<int>(request_id_) 
        << ", header=" << format_json_bytes(header_)
        << ", body=" << format_json_bytes(body_);
    return oss.str();
}

ProfileAdded::ProfileAdded(const Common& common, const MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileAddedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileAdded::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(256);
    return CIFactory::midiCIProfileAddedRemoved(dst, common_.address, false, common_.source_muid, profile_id_);
}

std::string ProfileAdded::getLabel() const {
    return "ProfileAdded";
}

std::string ProfileAdded::getBodyString() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size());
}

ProfileRemoved::ProfileRemoved(const Common& common, const MidiCIProfileId& profile_id)
    : SinglePacketMessage(MessageType::ProfileRemovedReport, common), profile_id_(profile_id) {}

std::vector<uint8_t> ProfileRemoved::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(256);
    return CIFactory::midiCIProfileAddedRemoved(dst, common_.address, true, common_.source_muid, profile_id_);
}

std::string ProfileRemoved::getLabel() const {
    return "ProfileRemoved";
}

std::string ProfileRemoved::getBodyString() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size());
}

ProfileEnabled::ProfileEnabled(const Common& common, const MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileEnabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileEnabled::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(256);
    return CIFactory::midiCIProfileReport(dst, common_.address, true, common_.source_muid, profile_id_, num_channels_);
}

std::string ProfileEnabled::getLabel() const {
    return "ProfileEnabled";
}

std::string ProfileEnabled::getBodyString() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDisabled::ProfileDisabled(const Common& common, const MidiCIProfileId& profile_id, uint16_t num_channels)
    : SinglePacketMessage(MessageType::ProfileDisabledReport, common), profile_id_(profile_id), num_channels_(num_channels) {}

std::vector<uint8_t> ProfileDisabled::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(256);
    return CIFactory::midiCIProfileReport(dst, common_.address, false, common_.source_muid, profile_id_, num_channels_);
}

std::string ProfileDisabled::getLabel() const {
    return "ProfileDisabled";
}

std::string ProfileDisabled::getBodyString() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", num_channels=" + std::to_string(num_channels_);
}

ProfileDetailsReply::ProfileDetailsReply(const Common& common, const MidiCIProfileId& profile_id,
                                        uint8_t target, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::ProfileInquiryReply, common), profile_id_(profile_id), target_(target), data_(data) {}

std::vector<uint8_t> ProfileDetailsReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> dst(256);
    return CIFactory::midiCIProfileDetailsReply(dst, common_.address, false, common_.source_muid, profile_id_, target_, data_);
}

std::string ProfileDetailsReply::getLabel() const {
    return "ProfileDetailsReply";
}

std::string ProfileDetailsReply::getBodyString() const {
    return "profile_id_size=" + std::to_string(profile_id_.data.size()) +
           ", target=" + std::to_string(target_) + 
           ", data_size=" + std::to_string(data_.size());
}

ProcessInquiryCapabilitiesReply::ProcessInquiryCapabilitiesReply(const Common& common, uint8_t supported_features)
    : SinglePacketMessage(MessageType::ProcessInquiryCapabilitiesReply, common), supported_features_(supported_features) {}

std::vector<uint8_t> ProcessInquiryCapabilitiesReply::serialize(const MidiCIDeviceConfiguration& config) const {
    return {supported_features_};
}

std::string ProcessInquiryCapabilitiesReply::getLabel() const {
    return "ProcessInquiryCapabilitiesReply";
}

std::string ProcessInquiryCapabilitiesReply::getBodyString() const {
    return "supported_features=" + std::to_string(supported_features_);
}

// MidiMessageReportReply implementation
MidiMessageReportReply::MidiMessageReportReply(const Common& common, uint8_t system_messages, uint8_t channel_controller_messages, uint8_t note_data_messages)
    : SinglePacketMessage(MessageType::MidiMessageReportInquiry, common), system_messages_(system_messages), channel_controller_messages_(channel_controller_messages), note_data_messages_(note_data_messages) {}

std::vector<uint8_t> MidiMessageReportReply::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return CIFactory::midiCIMidiMessageReportReply(data, common_.address, 
                                                        common_.source_muid, common_.destination_muid,
                                                        system_messages_, channel_controller_messages_, 
                                                        note_data_messages_);
}

std::string MidiMessageReportReply::getLabel() const {
    return "MidiMessageReportReply";
}

std::string MidiMessageReportReply::getBodyString() const {
    std::ostringstream oss;
    oss << "systemMessages=" << static_cast<int>(system_messages_) 
        << ", channelControllerMessages=" << static_cast<int>(channel_controller_messages_)
        << ", noteDataMessages=" << static_cast<int>(note_data_messages_);
    return oss.str();
}

// MidiMessageReportNotifyEnd implementation
MidiMessageReportNotifyEnd::MidiMessageReportNotifyEnd(const Common& common)
    : SinglePacketMessage(MessageType::MidiMessageReportInquiry, common) {}

std::vector<uint8_t> MidiMessageReportNotifyEnd::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32);
    
    return CIFactory::midiCIEndOfMidiMessage(data, common_.address, 
                                                  common_.source_muid, common_.destination_muid);
}

std::string MidiMessageReportNotifyEnd::getLabel() const {
    return "MidiMessageReportNotifyEnd";
}

std::string MidiMessageReportNotifyEnd::getBodyString() const {
    return "";
}

ProfileSpecificData::ProfileSpecificData(const Common& common, const MidiCIProfileId& profile_id, const std::vector<uint8_t>& data)
    : SinglePacketMessage(MessageType::ProfileInquiry, common), profile_id_(profile_id), data_(data) {}

std::vector<uint8_t> ProfileSpecificData::serialize(const MidiCIDeviceConfiguration& config) const {
    std::vector<uint8_t> data;
    data.resize(32 + data_.size());
    return CIFactory::midiCIProfileSpecificData(data, common_.address,
                                                      common_.source_muid, common_.destination_muid,
                                                      profile_id_, data_);
}

std::string ProfileSpecificData::getLabel() const {
    return "ProfileSpecificData";
}

std::string ProfileSpecificData::getBodyString() const {
    std::ostringstream oss;
    oss << "profileId=[" << std::hex;
    for (size_t i = 0; i < profile_id_.data.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "0x" << static_cast<int>(profile_id_.data[i]);
    }
    oss << "], dataSize=" << std::dec << data_.size();
    return oss.str();
}

} // namespace midi_ci
