
#include <gtest/gtest.h>
#include "midicci/messages/Message.hpp"
#include "midicci/profiles/MidiCIProfile.hpp"
#include "midicci/core/MidiCIConstants.hpp"

using namespace midicci::messages;
using namespace midicci::core::constants;

class MessageSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    Common common{0x12345678, 0x87654321, MIDI_CI_ADDRESS_FUNCTION_BLOCK, 0};
    midicci::core::DeviceDetails device_info{0x7D, 0x00, 0x01, 0x01000000};
};

TEST_F(MessageSerializationTest, DiscoveryInquirySerialize) {
    DiscoveryInquiry inquiry(common, device_info, 0x7F, 512, 0);
    
    auto data = inquiry.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[0], MIDI_CI_UNIVERSAL_SYSEX_ID);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::DiscoveryInquiry));
}

TEST_F(MessageSerializationTest, SetProfileOnSerialize) {
    std::vector<uint8_t> profile_id = {0x7E, 0x00, 0x01, 0x02, 0x03};
    SetProfileOn set_on(common, midicci::profiles::MidiCIProfileId{profile_id}, 16);
    
    auto data = set_on.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::SetProfileOn));
    
    for (size_t i = 0; i < profile_id.size(); ++i) {
        EXPECT_EQ(data[13 + i], profile_id[i]);
    }
}

TEST_F(MessageSerializationTest, PropertyGetCapabilitiesSerialize) {
    PropertyGetCapabilities capabilities(common, 4);
    
    auto data = capabilities.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::PropertyGetCapabilities));
    EXPECT_EQ(data[13], 4);
}

TEST_F(MessageSerializationTest, GetPropertyDataSerialize) {
    std::vector<uint8_t> header = {0x01, 0x02, 0x03, 0x04};
    GetPropertyData get_data(common, 1, header);
    
    auto data = get_data.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::GetPropertyData));
    EXPECT_EQ(data[13], 1);
    
    uint16_t header_size = data[14] | (data[15] << 7);
    EXPECT_EQ(header_size, header.size());
}

TEST_F(MessageSerializationTest, EndpointInquirySerialize) {
    EndpointInquiry inquiry(common, 0x01);
    
    auto data = inquiry.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::EndpointInquiry));
    EXPECT_EQ(data[13], 0x01);
}

TEST_F(MessageSerializationTest, InvalidateMUIDSerialize) {
    InvalidateMUID invalidate(common, 0xDEADBEEF);
    
    auto data = invalidate.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::InvalidateMUID));
    
    uint32_t target_muid = data[13] | (data[14] << 8) | (data[15] << 16) | (data[16] << 24);
    EXPECT_EQ(target_muid, 0xDEADBEEF);
}

TEST_F(MessageSerializationTest, ProfileInquirySerialize) {
    ProfileInquiry inquiry(common);
    
    auto data = inquiry.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::ProfileInquiry));
}

TEST_F(MessageSerializationTest, MidiMessageReportInquirySerialize) {
    MidiMessageReportInquiry inquiry(common, 0x01, 0x02, 0x03, 0x04);
    
    auto data = inquiry.serialize();
    EXPECT_GT(data.size(), 0);
    EXPECT_EQ(data[3], static_cast<uint8_t>(MessageType::MidiMessageReportInquiry));
    EXPECT_EQ(data[13], 0x01);
    EXPECT_EQ(data[14], 0x02);
    EXPECT_EQ(data[15], 0x03);
    EXPECT_EQ(data[16], 0x04);
}
