#pragma once

#include <cstdint>
#include <vector>
#include <memory>
#include <string>

namespace midi_ci {
namespace messages {

enum class MessageType : uint8_t {
    DiscoveryInquiry = 0x70,
    DiscoveryReply = 0x71,
    EndpointInquiry = 0x72,
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
    GetPropertyData = 0x34,
    SetPropertyData = 0x35,
    SubscribeProperty = 0x36,
    ProcessInquiryCapabilities = 0x40,
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
    
    virtual std::vector<uint8_t> serialize() const = 0;
    virtual bool deserialize(const std::vector<uint8_t>& data) = 0;
    virtual std::string get_label() const = 0;
    virtual std::string get_body_string() const = 0;
    
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
    DiscoveryInquiry(const Common& common, const DeviceInfo& device_info, 
                    uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    DeviceInfo device_info_;
    uint8_t supported_features_;
    uint32_t max_sysex_size_;
    uint8_t output_path_id_;
};

class DiscoveryReply : public SinglePacketMessage {
public:
    DiscoveryReply(const Common& common, const DeviceInfo& device_info, 
                  uint8_t supported_features, uint32_t max_sysex_size, uint8_t output_path_id, uint8_t function_block);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    DeviceInfo device_info_;
    uint8_t supported_features_;
    uint32_t max_sysex_size_;
    uint8_t output_path_id_;
    uint8_t function_block_;
};

class SetProfileOn : public SinglePacketMessage {
public:
    SetProfileOn(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
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
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t max_simultaneous_requests_;
};

class GetPropertyData : public MultiPacketMessage {
public:
    GetPropertyData(const Common& common, uint8_t request_id, const std::vector<uint8_t>& header);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
};

class SetPropertyData : public MultiPacketMessage {
public:
    SetPropertyData(const Common& common, uint8_t request_id, 
                   const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
};

class SubscribeProperty : public MultiPacketMessage {
public:
    SubscribeProperty(const Common& common, uint8_t request_id, 
                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t request_id_;
    std::vector<uint8_t> header_;
    std::vector<uint8_t> body_;
};

class EndpointInquiry : public SinglePacketMessage {
public:
    EndpointInquiry(const Common& common, uint8_t status);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint8_t status_;
};

class InvalidateMUID : public SinglePacketMessage {
public:
    InvalidateMUID(const Common& common, uint32_t target_muid);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    uint32_t target_muid_;
};

class ProfileInquiry : public SinglePacketMessage {
public:
    ProfileInquiry(const Common& common);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
};

class SetProfileOff : public SinglePacketMessage {
public:
    SetProfileOff(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileEnabledReport : public SinglePacketMessage {
public:
    ProfileEnabledReport(const Common& common, const std::vector<uint8_t>& profile_id, uint16_t num_channels);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
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
    bool deserialize(const std::vector<uint8_t>& data) override;
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
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
    
private:
    std::vector<uint8_t> profile_id_;
};

class ProfileRemovedReport : public SinglePacketMessage {
public:
    ProfileRemovedReport(const Common& common, const std::vector<uint8_t>& profile_id);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
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
    bool deserialize(const std::vector<uint8_t>& data) override;
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
    bool deserialize(const std::vector<uint8_t>& data) override;
    std::string get_label() const override;
    std::string get_body_string() const override;
};

} // namespace messages
} // namespace midi_ci
