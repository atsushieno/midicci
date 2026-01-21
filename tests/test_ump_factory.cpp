#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace umppi;

class UmpFactoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed for UMP tests
    }
};

// Test basic UMP Factory functionality for SysEx7
TEST_F(UmpFactoryTest, testSysEx7Direct) {
    auto ump = UmpFactory::sysex7Direct(1, 0, 6, 0x41, 0x10, 0x42, 0x40, 0x00, 0x7F);
    EXPECT_EQ(0x310641104240007F, ((uint64_t)ump.int1 << 32) | ump.int2);
}

TEST_F(UmpFactoryTest, testSysEx7GetSysexLength) {
    // Test with 0xF0 prefix
    std::vector<uint8_t> gsReset = {0xF0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
    EXPECT_EQ(9, UmpFactory::sysex7GetSysexLength(gsReset));
    
    // Test without 0xF0 prefix (should get same result)
    std::vector<uint8_t> gsResetNoF0 = {0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41, 0xF7};
    EXPECT_EQ(9, UmpFactory::sysex7GetSysexLength(gsResetNoF0));
    
    // Test various lengths
    std::vector<uint8_t> sysex12 = {0xF0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xF7};
    EXPECT_EQ(12, UmpFactory::sysex7GetSysexLength(sysex12));
    
    std::vector<uint8_t> sysex13 = {0xF0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0xF7};
    EXPECT_EQ(13, UmpFactory::sysex7GetSysexLength(sysex13));
}

TEST_F(UmpFactoryTest, testSysEx7GetPacketCount) {
    EXPECT_EQ(1, UmpFactory::sysex7GetPacketCount({0}));
    EXPECT_EQ(1, UmpFactory::sysex7GetPacketCount({0, 0}));
    
    // Test data that needs multiple packets (6 bytes per packet for SysEx7)
    std::vector<uint8_t> sysex7bytes = {0, 1, 2, 3, 4, 5, 6, 0xF7};
    EXPECT_EQ(2, UmpFactory::sysex7GetPacketCount(sysex7bytes));
    
    std::vector<uint8_t> sysex12bytes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xF7};
    EXPECT_EQ(2, UmpFactory::sysex7GetPacketCount(sysex12bytes));
    
    std::vector<uint8_t> sysex13bytes = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0xF7};
    EXPECT_EQ(3, UmpFactory::sysex7GetPacketCount(sysex13bytes));
}

// Note: More comprehensive tests would need additional UmpFactory methods to be implemented
// This test will serve as a starting point and can be expanded as more functionality is added

TEST_F(UmpFactoryTest, testSysEx7Process) {
    std::vector<uint8_t> sysex6 = {1, 2, 3, 4, 5, 6};
    std::vector<Ump> packets;
    
    UmpFactory::sysex7Process(0, sysex6, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });
    
    EXPECT_EQ(1, packets.size());
    
    // Extract UMP back to SysEx data and verify
    auto retrieved = UmpRetriever::getSysex7Data(packets);
    ASSERT_EQ(sysex6.size(), retrieved.size());
    for (size_t i = 0; i < sysex6.size(); ++i) {
        EXPECT_EQ(sysex6[i], retrieved[i]) << "Mismatch at index " << i;
    }
}

// Test some of the newly implemented UmpFactory methods
TEST_F(UmpFactoryTest, testBasicUmpFactoryMethods) {
    // Test utility messages
    EXPECT_EQ(0, UmpFactory::noop());
    EXPECT_EQ(0x00100000, UmpFactory::jrClock(uint16_t(0)));
    EXPECT_EQ(0x00107A12, UmpFactory::jrClock(1.0));
    EXPECT_EQ(0x00200000, UmpFactory::jrTimestamp(uint16_t(0)));
    EXPECT_EQ(0x00207A12, UmpFactory::jrTimestamp(1.0));
    
    // Test system messages  
    EXPECT_EQ(0x11F16300, UmpFactory::systemMessage(1, 0xF1, 99, 0));
    EXPECT_EQ(0x11F26359, UmpFactory::systemMessage(1, 0xF2, 99, 89));
    EXPECT_EQ(0x11FF0000, UmpFactory::systemMessage(1, 0xFF, 0, 0));
    
    // Test MIDI1 messages
    EXPECT_EQ(0x2182410A, UmpFactory::midi1NoteOff(1, 2, 65, 10));
    EXPECT_EQ(0x2192410A, UmpFactory::midi1NoteOn(1, 2, 65, 10));
    EXPECT_EQ(0x21A2410A, UmpFactory::midi1PAf(1, 2, 65, 10));
    EXPECT_EQ(0x21B2410A, UmpFactory::midi1CC(1, 2, 65, 10));
    EXPECT_EQ(0x21C21D00, UmpFactory::midi1Program(1, 2, 29));
    EXPECT_EQ(0x21D20A00, UmpFactory::midi1CAf(1, 2, 10));
    EXPECT_EQ(0x21E20000, UmpFactory::midi1PitchBendDirect(1, 2, 0));
    EXPECT_EQ(0x21E20100, UmpFactory::midi1PitchBendDirect(1, 2, 1));
    EXPECT_EQ(0x21E27F7F, UmpFactory::midi1PitchBendDirect(1, 2, 0x3FFF));
    EXPECT_EQ(0x21E20040, UmpFactory::midi1PitchBend(1, 2, 0));
    EXPECT_EQ(0x21E20000, UmpFactory::midi1PitchBend(1, 2, -8192));
    EXPECT_EQ(0x21E27F7F, UmpFactory::midi1PitchBend(1, 2, 8191));
}

TEST_F(UmpFactoryTest, testMidi2Messages) {
    // Test pitch functions
    auto pitch = UmpFactory::pitch7_9Split(0x20, 0.5);
    EXPECT_EQ(0x4100, pitch);
    pitch = UmpFactory::pitch7_9(32.5);
    EXPECT_EQ(0x4100, pitch);
    
    // Test MIDI2 channel messages
    auto v = UmpFactory::midi2NoteOff(1, 2, 0x20, MidiNoteAttributeType::Pitch7_9, 0xFEDC, pitch);
    EXPECT_EQ(0x41822003FEDC4100ULL, v);
    
    v = UmpFactory::midi2NoteOff(1, 2, 0x20, MidiNoteAttributeType::Pitch7_9, 0x1234, 0);
    EXPECT_EQ(0x4182200312340000ULL, v);
    
    v = UmpFactory::midi2NoteOn(1, 2, 64, 0, 0xFEDC, 0);
    EXPECT_EQ(0x41924000FEDC0000ULL, v);
    
    v = UmpFactory::midi2PAf(1, 2, 64, 0x87654321);
    EXPECT_EQ(0x41A2400087654321ULL, v);
    
    v = UmpFactory::midi2CC(1, 2, 1, 0x87654321);
    EXPECT_EQ(0x41B2010087654321ULL, v);
    
    v = UmpFactory::midi2Program(1, 2, 1, 29, 8, 1);
    EXPECT_EQ(0x41C200011D000801ULL, v);
    
    v = UmpFactory::midi2CAf(1, 2, 0x87654321);
    EXPECT_EQ(0x41D2000087654321ULL, v);
    
    v = UmpFactory::midi2PitchBendDirect(1, 2, 0x87654321);
    EXPECT_EQ(0x41E2000087654321ULL, v);
    
    v = UmpFactory::midi2PitchBend(1, 2, 1);
    EXPECT_EQ(0x41E2000080000001ULL, v);
    
    v = UmpFactory::midi2RPN(1, 2, 0x10, 0x20, 0x12345678);
    EXPECT_EQ(0x4122102012345678ULL, v);
    
    v = UmpFactory::midi2NRPN(1, 2, 0x10, 0x20, 0x12345678);
    EXPECT_EQ(0x4132102012345678ULL, v);
}