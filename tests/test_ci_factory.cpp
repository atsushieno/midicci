#include <gtest/gtest.h>
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include <vector>

using namespace midi_ci::messages;
using namespace midi_ci::core::constants;

class CIFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    uint32_t midiCI32to28(uint32_t value) {
        return ((value & 0x7F) << 0) |
               ((value & 0x7F00) >> 1) |
               ((value & 0x7F0000) >> 2) |
               ((value & 0x7F000000) >> 3);
    }
};

TEST_F(CIFactoryTest, testDiscoveryMessages) {
    Common common1(0x10101010, 0x7F7F7F7F, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 1);
    DeviceInfo device_info{"TestMfg", "TestFamily", "TestModel", "1.0"};
    
    DiscoveryInquiry inquiry(common1, device_info, 0x1C, 512, 0);
    auto data1 = inquiry.serialize();
    
    EXPECT_EQ(data1[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data1[3], static_cast<uint8_t>(MessageType::DiscoveryInquiry));
    
    Common common2(0x10101010, 0x20202020, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 1);
    DiscoveryReply reply(common2, device_info, 0x1C, 512, 0, 0);
    auto data2 = reply.serialize();
    
    EXPECT_EQ(data2[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data2[3], static_cast<uint8_t>(MessageType::DiscoveryReply));
    
    Common common3(0x10101010, 0x7F7F7F7F, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 1);
    InvalidateMUID invalidate(common3, 0x20202020);
    auto data3 = invalidate.serialize();
    
    EXPECT_EQ(data3[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data3[3], static_cast<uint8_t>(MessageType::InvalidateMUID));
}

TEST_F(CIFactoryTest, testProfileConfigurationMessages) {
    Common common(0x10101010, 0x20202020, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 5);
    ProfileInquiry inquiry(common);
    auto data = inquiry.serialize();
    
    EXPECT_EQ(data[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::ProfileInquiry));
}

TEST_F(CIFactoryTest, midiCI32to28Test) {
    EXPECT_EQ(0xFFFFFFF, midiCI32to28(0x7F7F7F7F));
    EXPECT_EQ(0xFC285E9, midiCI32to28(0x7E0A0B69));
    EXPECT_EQ(0xCBD8657, midiCI32to28(0x65760C57));
}

TEST_F(CIFactoryTest, testPropertyExchangeMessages) {
    Common common(0x10101010, 0x20202020, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 5);
    
    PropertyGetCapabilities inquiry(common, 16);
    auto data1 = inquiry.serialize();
    
    EXPECT_EQ(data1[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data1[3], static_cast<uint8_t>(MessageType::PropertyGetCapabilities));
    
    std::vector<uint8_t> header = {11, 22, 33, 44};
    std::vector<uint8_t> data = {55, 66, 77, 88, 99};
    
    GetPropertyData getInquiry(common, 2, header);
    auto data3 = getInquiry.serialize();
    EXPECT_EQ(data3[3], static_cast<uint8_t>(MessageType::GetPropertyData));
    
    GetPropertyDataReply getReply(common, 2, header, data);
    auto data4 = getReply.serialize();
    EXPECT_EQ(data4[3], static_cast<uint8_t>(MessageType::GetPropertyDataReply));
    
    SetPropertyData setInquiry(common, 2, header, data);
    auto data5 = setInquiry.serialize();
    EXPECT_EQ(data5[3], static_cast<uint8_t>(MessageType::SetPropertyData));
    
    SetPropertyDataReply setReply(common, 2, header);
    auto data6 = setReply.serialize();
    EXPECT_EQ(data6[3], static_cast<uint8_t>(MessageType::SetPropertyDataReply));
    
    SubscribeProperty subscribe(common, 2, header, data);
    auto data7 = subscribe.serialize();
    EXPECT_EQ(data7[3], static_cast<uint8_t>(MessageType::SubscribeProperty));
    
    SubscribePropertyReply subscribeReply(common, 2, header, data);
    auto data8 = subscribeReply.serialize();
    EXPECT_EQ(data8[3], static_cast<uint8_t>(MessageType::SubscribePropertyReply));
}
