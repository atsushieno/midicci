#pragma once

#include <cstdint>
#include <vector>
#include <memory>

namespace midi_ci {
namespace messages {

enum class MessageType : uint8_t {
    DiscoveryInquiry = 0x70,
    DiscoveryReply = 0x71,
    ProfileInquiry = 0x20,
    ProfileInquiryReply = 0x21,
    PropertyExchangeGet = 0x34,
    PropertyExchangeGetReply = 0x35,
    Ack = 0x7D,
    Nak = 0x7F
};

class Message {
public:
    Message(MessageType type, uint32_t source_muid, uint32_t destination_muid);
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
    
protected:
    MessageType type_;
    uint32_t source_muid_;
    uint32_t destination_muid_;
};

class DiscoveryInquiry : public Message {
public:
    DiscoveryInquiry(uint32_t source_muid, uint32_t destination_muid);
    
    std::vector<uint8_t> serialize() const override;
    bool deserialize(const std::vector<uint8_t>& data) override;
};

} // namespace messages
} // namespace midi_ci
