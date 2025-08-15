#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace midicci::ump;

class UmpTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed for UMP tests
    }
};

TEST_F(UmpTest, testBasicUmpConstruction) {
    Ump ump1(0x44914000, 0x64000000);  // Group=4 (bits 27-24), MessageType=4 (bits 31-28)
    EXPECT_EQ(midicci::ump::MessageType::MIDI2, ump1.get_message_type());
    EXPECT_EQ(4, ump1.get_group());
}

TEST_F(UmpTest, testMessageTypeDetection) {
    Ump utility(static_cast<uint32_t>(0x00000000));
    EXPECT_EQ(midicci::ump::MessageType::UTILITY, utility.get_message_type());
    
    Ump system(static_cast<uint32_t>(0x10000000));
    EXPECT_EQ(midicci::ump::MessageType::SYSTEM, system.get_message_type());
    
    Ump midi1(static_cast<uint32_t>(0x20000000));
    EXPECT_EQ(midicci::ump::MessageType::MIDI1, midi1.get_message_type());
    
    Ump sysex7(static_cast<uint32_t>(0x30000000));
    EXPECT_EQ(midicci::ump::MessageType::SYSEX7, sysex7.get_message_type());
    
    Ump midi2(static_cast<uint32_t>(0x40000000));
    EXPECT_EQ(midicci::ump::MessageType::MIDI2, midi2.get_message_type());
    
    Ump sysex8(static_cast<uint32_t>(0x50000000));
    EXPECT_EQ(midicci::ump::MessageType::SYSEX8_MDS, sysex8.get_message_type());
}

TEST_F(UmpTest, testSizeInBytes) {
    Ump utility(static_cast<uint32_t>(0x00000000));
    EXPECT_EQ(4, utility.get_size_in_bytes());
    
    Ump midi2(static_cast<uint32_t>(0x40000000));
    EXPECT_EQ(8, midi2.get_size_in_bytes());
    
    Ump sysex8(static_cast<uint32_t>(0x50000000));
    EXPECT_EQ(16, sysex8.get_size_in_bytes());
    
    Ump flexData(static_cast<uint32_t>(0xD0000000));
    EXPECT_EQ(16, flexData.get_size_in_bytes());
}

// More comprehensive tests would need additional UMP functionality
TEST_F(UmpTest, DISABLED_testNeedsMoreMethods) {
    // These tests would require implementing additional methods like:
    // - toPlatformBytes()
    // - fromBytes()
    // - MIDI channel/note/velocity accessors
    // - ByteOrder handling
    
    GTEST_SKIP() << "Requires implementing more Ump methods";
}