#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace umppi;

class UmpTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed for UMP tests
    }
};

TEST_F(UmpTest, testBasicUmpConstruction) {
    Ump ump1(0x44914000, 0x64000000);  // Group=4 (bits 27-24), MessageType=4 (bits 31-28)
    EXPECT_EQ(umppi::MessageType::MIDI2, ump1.getMessageType());
    EXPECT_EQ(4, ump1.getGroup());
}

TEST_F(UmpTest, testMessageTypeDetection) {
    Ump utility(static_cast<uint32_t>(0x00000000));
    EXPECT_EQ(umppi::MessageType::UTILITY, utility.getMessageType());
    
    Ump system(static_cast<uint32_t>(0x10000000));
    EXPECT_EQ(umppi::MessageType::SYSTEM, system.getMessageType());
    
    Ump midi1(static_cast<uint32_t>(0x20000000));
    EXPECT_EQ(umppi::MessageType::MIDI1, midi1.getMessageType());
    
    Ump sysex7(static_cast<uint32_t>(0x30000000));
    EXPECT_EQ(umppi::MessageType::SYSEX7, sysex7.getMessageType());
    
    Ump midi2(static_cast<uint32_t>(0x40000000));
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2.getMessageType());
    
    Ump sysex8(static_cast<uint32_t>(0x50000000));
    EXPECT_EQ(umppi::MessageType::SYSEX8_MDS, sysex8.getMessageType());
}

TEST_F(UmpTest, testSizeInBytes) {
    Ump utility(static_cast<uint32_t>(0x00000000));
    EXPECT_EQ(4, utility.getSizeInBytes());
    
    Ump midi2(static_cast<uint32_t>(0x40000000));
    EXPECT_EQ(8, midi2.getSizeInBytes());
    
    Ump sysex8(static_cast<uint32_t>(0x50000000));
    EXPECT_EQ(16, sysex8.getSizeInBytes());
    
    Ump flexData(static_cast<uint32_t>(0xD0000000));
    EXPECT_EQ(16, flexData.getSizeInBytes());
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