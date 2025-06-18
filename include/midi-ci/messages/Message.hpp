#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>
#include "midi-ci/core/MidiCIConstants.hpp"

namespace midi_ci {
namespace messages {

enum class MessageType : uint8_t {
    DiscoveryInquiry = 0x70,
    DiscoveryReply = 0x71,
    EndpointInquiry = 0x72,
    EndpointReply = 0x73,
    InvalidateMUID = 0x7E,
    ProfileInquiry = 0x20,
    ProfileInquiryReply = 0x21,
    SetProfileOn = 0x22,
    SetProfileOff = 0x23,
    ProfileEnabledReport = 0x24,
    ProfileDisabledReport = 0x25,
    ProfileAddedReport = 0x26,
    ProfileRemovedReport = 0x27,
    PropertyGetCapabilities = 0x30,
    PropertyGetCapabilitiesReply = 0x31,
    GetPropertyData = 0x34,
    GetPropertyDataReply = 0x35,
    SetPropertyData = 0x36,
    SetPropertyDataReply = 0x37,
    SubscribeProperty = 0x38,
    SubscribePropertyReply = 0x39,
    PropertyNotify = 0x3F,
    ProcessInquiryCapabilities = 0x40,
    ProcessInquiryCapabilitiesReply = 0x41,
    MidiMessageReportInquiry = 0x41
};

struct DeviceInfo {
    std::string manufacturer;
    std::string family;
    std::string model;
    std::string version;
    
    DeviceInfo(const std::string& mfg, const std::string& fam, const std::string& mod, const std::string& ver)
        : manufacturer(mfg), family(fam), model(mod), version(ver) {}
};

struct Common {
    uint32_t source_muid;
    uint32_t destination_muid;
    uint8_t address;
    uint8_t group;
    
    Common(uint32_t src, uint32_t dest, uint8_t addr, uint8_t grp)
        : source_muid(src), destination_muid(dest), address(addr), group(grp) {}
};

class Message {
public:
    Message(MessageType type, const Common& common);
    virtual ~Message() = default;
    
    Message(const Message&) = delete;
    Message& operator=(const Message&) = delete;
    
    Message(Message&&) = default;
    Message& operator=(Message&&) = default;
    
    MessageType get_type() const noexcept;
    uint32_t get_source_muid() const noexcept;
    uint32_t get_destination_muid() const noexcept;
    const Common& get_common() const noexcept { return common_; }
    
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual std::vector<std::vector<uint8_t>> serialize_multi() const;
    virtual std::string get_label() const = 0;
    virtual std::string get_body_string() const = 0;
    virtual std::string get_log_message() const;
    
protected:
    MessageType type_;
    Common common_;
};

class SinglePacketMessage : public Message {
public:
    SinglePacketMessage(MessageType type, const Common& common);
};

class MultiPacketMessage : public Message {
public:
    MultiPacketMessage(MessageType type, const Common& common);
};

class DiscoveryInquiry : public SinglePacketMessage {
public:
    DiscoveryInquiry(const Common& common, const core::DeviceDetails& device_details,
                    uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    core::DeviceDetails device_details_;
    uint8_t supported_features_;
    uint32_t max_sysex_size_;
    uint8_t output_path_id_;
};

class DiscoveryReply : public SinglePacketMessage {
public:
    DiscoveryReply(const Common& common, const core::DeviceDetails& device_details,
                  uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id, uint8_t function_block);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    core::DeviceDetails device_details_;
    uint8_t supported_features_;
    uint32_t max_sysex_size_;
    uint8_t output_path_id_;
    uint8_t function_block_;
};

class SetProfileOn : public SinglePacketMessage {
public:
    SetProfileOn(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
    uint16_t num_channels_;
};

class PropertyGetCapabilities : public SinglePacketMessage {
public:
    PropertyGetCapabilities(const Common& common, uint8_t max_simultaneous_requests);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_max_simultaneous_requests() const { return max_simultaneous_requests_; }
    
private:
    uint8_t max_simultaneous_requests_;
};

class GetPropertyData : public MultiPacketMessage {
public:
    GetPropertyData(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header);
    GetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, const std::string& res_id = "");
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    
    std::vector<uint8_t> create_json_header(const std::string& resource_identifier, const std::string& res_id, 
                                          const std::string& mutual_encoding, bool set_partial, 
                                          int offset, int limit) const;
};

class SetPropertyData : public MultiPacketMessage {
public:
    SetPropertyData(const Common& common, uint8_t request_id, 
                   const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    SetPropertyData(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                   const std::vector<uint8_t>& body, const std::string& res_id = "", bool set_partial = false);
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    const std::vector<uint8_t>& get_body() const { return body_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
    
    std::vector<uint8_t> create_json_header(const std::string& resource_identifier, const std::string& res_id, 
                                          const std::string& mutual_encoding, bool set_partial, 
                                          int offset, int limit) const;
};

class SubscribeProperty : public MultiPacketMessage {
public:
    SubscribeProperty(const Common& common, uint8_t request_id, 
                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    SubscribeProperty(const Common& common, uint8_t request_id, const std::string& resource_identifier, 
                     const std::string& command, const std::string& mutual_encoding = "");
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    const std::vector<uint8_t>& get_body() const { return body_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
    
    std::vector<uint8_t> create_subscribe_json_header(const std::string& resource_identifier, 
                                                    const std::string& command, 
                                                    const std::string& mutual_encoding) const;
};

class EndpointInquiry : public SinglePacketMessage {
public:
    EndpointInquiry(const Common& common, uint8_t status);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_status() const { return status_; }
    
private:
    uint8_t status_;
};

class EndpointReply : public SinglePacketMessage {
public:
    EndpointReply(const Common& common, uint8_t status, const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_status() const { return status_; }
    const std::vector<uint8_t>& get_data() const { return data_; }
    
private:
    uint8_t status_;
    std::vector<uint8_t> data_;
};

class InvalidateMUID : public SinglePacketMessage {
public:
    InvalidateMUID(const Common& common, uint32_t target_muid);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint32_t target_muid_;
};

class ProfileInquiry : public SinglePacketMessage {
public:
    ProfileInquiry(const Common& common);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
};

class SetProfileOff : public SinglePacketMessage {
public:
    SetProfileOff(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileEnabledReport : public SinglePacketMessage {
public:
    ProfileEnabledReport(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
    uint16_t num_channels_;
};

class ProfileDisabledReport : public SinglePacketMessage {
public:
    ProfileDisabledReport(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
    uint16_t num_channels_;
};

class ProfileAddedReport : public SinglePacketMessage {
public:
    ProfileAddedReport(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileRemovedReport : public SinglePacketMessage {
public:
    ProfileRemovedReport(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
};

class MidiMessageReportInquiry : public SinglePacketMessage {
public:
    MidiMessageReportInquiry(const Common& common, uint8_t message_data_control,
                           uint8_t system_messages, uint8_t channel_controller_messages,
                           uint8_t note_data_messages);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t message_data_control_;
    uint8_t system_messages_;
    uint8_t channel_controller_messages_;
    uint8_t note_data_messages_;
};

class ProcessInquiryCapabilities : public SinglePacketMessage {
public:
    ProcessInquiryCapabilities(const Common& common);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
};

class ProfileReply : public SinglePacketMessage {
public:
    ProfileReply(const Common& common, const std::vector<std::vector<uint8_t>>& enabled_profiles, 
                const std::vector<std::vector<uint8_t>>& disabled_profiles);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<std::vector<uint8_t>>& get_enabled_profiles() const { return enabled_profiles_; }
    const std::vector<std::vector<uint8_t>>& get_disabled_profiles() const { return disabled_profiles_; }
    
private:
    std::vector<std::vector<uint8_t>> enabled_profiles_;
    std::vector<std::vector<uint8_t>> disabled_profiles_;
};

class PropertyGetCapabilitiesReply : public SinglePacketMessage {
public:
    PropertyGetCapabilitiesReply(const Common& common, uint8_t max_simultaneous_requests);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_max_simultaneous_requests() const { return max_simultaneous_requests_; }
    
private:
    uint8_t max_simultaneous_requests_;
};

class GetPropertyDataReply : public MultiPacketMessage {
public:
    GetPropertyDataReply(const Common& common, uint8_t request_id, 
                        const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    const std::vector<uint8_t>& get_body() const { return body_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
};

class SetPropertyDataReply : public MultiPacketMessage {
public:
    SetPropertyDataReply(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header);
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
};

class SubscribePropertyReply : public MultiPacketMessage {
public:
    SubscribePropertyReply(const Common& common, uint8_t request_id, 
                          const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    std::vector<uint8_t> serialize() const override;
    std::vector<std::vector<uint8_t>> serialize_multi() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_request_id() const { return request_id_; }
    const std::vector<uint8_t>& get_header() const { return header_; }
    const std::vector<uint8_t>& get_body() const { return body_; }
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
};

class ProfileAdded : public SinglePacketMessage {
public:
    ProfileAdded(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileRemoved : public SinglePacketMessage {
public:
    ProfileRemoved(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileEnabled : public SinglePacketMessage {
public:
    ProfileEnabled(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    uint16_t get_num_channels() const { return num_channels_; }
    
private:
    std::vector<uint8_t> profile_id_;
    uint16_t num_channels_;
};

class ProfileDisabled : public SinglePacketMessage {
public:
    ProfileDisabled(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    uint16_t get_num_channels() const { return num_channels_; }
    
private:
    std::vector<uint8_t> profile_id_;
    uint16_t num_channels_;
};

class ProfileDetailsReply : public SinglePacketMessage {
public:
    ProfileDetailsReply(const Common& common, const std::vector<uint8_t>& profile_id, 
                       uint8_t target, const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    uint8_t get_target() const { return target_; }
    const std::vector<uint8_t>& get_data() const { return data_; }
    
private:
    std::vector<uint8_t> profile_id_;
    uint8_t target_;
    std::vector<uint8_t> data_;
};

class ProcessInquiryCapabilitiesReply : public SinglePacketMessage {
public:
    ProcessInquiryCapabilitiesReply(const Common& common, uint8_t supported_features);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_supported_features() const { return supported_features_; }
    
private:
    uint8_t supported_features_;
};

class MidiMessageReportReply : public SinglePacketMessage {
public:
    MidiMessageReportReply(const Common& common, uint8_t system_messages, uint8_t channel_controller_messages, uint8_t note_data_messages);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    uint8_t get_system_messages() const { return system_messages_; }
    uint8_t get_channel_controller_messages() const { return channel_controller_messages_; }
    uint8_t get_note_data_messages() const { return note_data_messages_; }
    
private:
    uint8_t system_messages_;
    uint8_t channel_controller_messages_;
    uint8_t note_data_messages_;
};

class MidiMessageReportNotifyEnd : public SinglePacketMessage {
public:
    MidiMessageReportNotifyEnd(const Common& common);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
};

class ProfileSpecificData : public SinglePacketMessage {
public:
    ProfileSpecificData(const Common& common, const std::vector<uint8_t>& profile_id, const std::vector<uint8_t>& data);
    
    std::vector<uint8_t> serialize() const override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
    const std::vector<uint8_t>& get_profile_id() const { return profile_id_; }
    const std::vector<uint8_t>& get_data() const { return data_; }
    
private:
    std::vector<uint8_t> profile_id_;
    std::vector<uint8_t> data_;
};

} // namespace messages
} // namespace midi_ci
