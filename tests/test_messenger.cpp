#include <gtest/gtest.h>
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/messages/Message.hpp"

using namespace midi_ci;

class MessengerTest : public ::testing::Test {
protected:
    void SetUp() override {
        device = std::make_unique<core::MidiCIDevice>(0x12345678);
        device->set_device_info({"TestMfg", "TestFamily", "TestModel", "1.0"});
    }
    
    std::unique_ptr<core::MidiCIDevice> device;
};

TEST_F(MessengerTest, SendDiscoveryInquiry) {
    bool message_received = false;
    device->add_message_callback([&](const messages::Message& msg) {
        EXPECT_EQ(msg.get_type(), messages::MessageType::DiscoveryInquiry);
        EXPECT_EQ(msg.get_source_muid(), 0x12345678);
        message_received = true;
    });
    
    device->send_discovery_inquiry();
    EXPECT_TRUE(message_received);
}

TEST_F(MessengerTest, GetNextRequestId) {
    auto& messenger = device->get_messenger();
    uint8_t id1 = messenger.get_next_request_id();
    uint8_t id2 = messenger.get_next_request_id();
    EXPECT_NE(id1, id2);
}
