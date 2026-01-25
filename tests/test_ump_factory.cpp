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

    v = UmpFactory::midi2PerNoteRCC(1, 2, 56, 0x10, 0x33333333);
    EXPECT_EQ(0x4102381033333333ULL, v);

    v = UmpFactory::midi2PerNoteACC(1, 2, 56, 0x10, 0x33333333);
    EXPECT_EQ(0x4112381033333333ULL, v);

    v = UmpFactory::midi2RelativeRPN(1, 2, 0x10, 0x20, 0x12345678);
    EXPECT_EQ(0x4142102012345678ULL, v);

    v = UmpFactory::midi2RelativeNRPN(1, 2, 0x10, 0x20, 0x12345678);
    EXPECT_EQ(0x4152102012345678ULL, v);

    v = UmpFactory::midi2PerNotePitchBendDirect(1, 2, 56, 0x87654321);
    EXPECT_EQ(0x4162380087654321ULL, v);

    v = UmpFactory::midi2PerNotePitchBend(1, 2, 56, 1);
    EXPECT_EQ(0x4162380080000001ULL, v);

    v = UmpFactory::midi2PerNoteManagement(1, 2, 56, 2);
    EXPECT_EQ(0x41F2380200000000ULL, v);
}

TEST_F(UmpFactoryTest, testSysEx8) {
    std::vector<uint8_t> gsReset = {0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7F, 0x00, 0x41};
    std::vector<uint8_t> sysex27(27);
    for (int i = 0; i < 27; i++) sysex27[i] = i + 1;

    EXPECT_EQ(1, UmpFactory::sysex8GetPacketCount(0));
    EXPECT_EQ(1, UmpFactory::sysex8GetPacketCount(1));
    EXPECT_EQ(1, UmpFactory::sysex8GetPacketCount(13));
    EXPECT_EQ(2, UmpFactory::sysex8GetPacketCount(14));
    EXPECT_EQ(2, UmpFactory::sysex8GetPacketCount(26));
    EXPECT_EQ(3, UmpFactory::sysex8GetPacketCount(27));

    auto packet = UmpFactory::sysex8GetPacketOf(1, 7, gsReset, 0);
    EXPECT_EQ(0x510A0741U, packet.int1);
    EXPECT_EQ(0x10421240U, packet.int2);
    EXPECT_EQ(0x007F0041U, packet.int3);
    EXPECT_EQ(0x00000000U, packet.int4);

    packet = UmpFactory::sysex8GetPacketOf(1, 7, sysex27, 0);
    EXPECT_EQ(0x511E0701U, packet.int1);
    EXPECT_EQ(0x02030405U, packet.int2);
    EXPECT_EQ(0x06070809U, packet.int3);
    EXPECT_EQ(0x0A0B0C0DU, packet.int4);

    packet = UmpFactory::sysex8GetPacketOf(1, 7, sysex27, 1);
    EXPECT_EQ(0x512E070EU, packet.int1);
    EXPECT_EQ(0x0F101112U, packet.int2);
    EXPECT_EQ(0x13141516U, packet.int3);
    EXPECT_EQ(0x1718191AU, packet.int4);

    packet = UmpFactory::sysex8GetPacketOf(1, 7, sysex27, 2);
    EXPECT_EQ(0x5132071BU, packet.int1);
    EXPECT_EQ(0x00000000U, packet.int2);
    EXPECT_EQ(0x00000000U, packet.int3);
    EXPECT_EQ(0x00000000U, packet.int4);
}

TEST_F(UmpFactoryTest, testSysEx8Process) {
    std::vector<uint8_t> sysex1 = {1};
    std::vector<Ump> packets;

    UmpFactory::sysex8Process(0, sysex1, 0, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });

    EXPECT_EQ(1, packets.size());
    EXPECT_EQ(0x50020001U, packets[0].int1);
    EXPECT_EQ(0U, packets[0].int2);

    std::vector<uint8_t> sysex13(13);
    for (int i = 0; i < 13; i++) sysex13[i] = i + 1;
    packets.clear();

    UmpFactory::sysex8Process(0, sysex13, 0, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });

    EXPECT_EQ(1, packets.size());
    EXPECT_EQ(0x500E0001U, packets[0].int1);
    EXPECT_EQ(0x02030405U, packets[0].int2);
    EXPECT_EQ(0x06070809U, packets[0].int3);
    EXPECT_EQ(0x0A0B0C0DU, packets[0].int4);

    std::vector<uint8_t> sysex14(14);
    for (int i = 0; i < 14; i++) sysex14[i] = i + 1;
    packets.clear();

    UmpFactory::sysex8Process(0, sysex14, 0, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });

    EXPECT_EQ(2, packets.size());
    EXPECT_EQ(0x501E0001U, packets[0].int1);
    EXPECT_EQ(0x02030405U, packets[0].int2);
    EXPECT_EQ(0x06070809U, packets[0].int3);
    EXPECT_EQ(0x0A0B0C0DU, packets[0].int4);
    EXPECT_EQ(0x5032000EU, packets[1].int1);
    EXPECT_EQ(0U, packets[1].int2);
    EXPECT_EQ(0U, packets[1].int3);
    EXPECT_EQ(0U, packets[1].int4);
}

TEST_F(UmpFactoryTest, testTempo) {
    auto tempo1 = UmpFactory::tempo(0, 0, 50000000);
    EXPECT_EQ(0xD0100000U, tempo1.int1);
    EXPECT_EQ(0x02FAF080U, tempo1.int2);
    EXPECT_EQ(0U, tempo1.int3);
    EXPECT_EQ(0U, tempo1.int4);

    auto tempo2 = UmpFactory::tempo(0xF, 0xE, 50000000);
    EXPECT_EQ(0xDF1E0000U, tempo2.int1);
    EXPECT_EQ(0x02FAF080U, tempo2.int2);
    EXPECT_EQ(0U, tempo2.int3);
    EXPECT_EQ(0U, tempo2.int4);
}

TEST_F(UmpFactoryTest, testTimeSignatureDirect) {
    auto ts1 = UmpFactory::timeSignatureDirect(0, 0, 3, 4, 0);
    EXPECT_EQ(0xD0100001U, ts1.int1);
    EXPECT_EQ(0x03040000U, ts1.int2);
    EXPECT_EQ(0U, ts1.int3);
    EXPECT_EQ(0U, ts1.int4);

    auto ts2 = UmpFactory::timeSignatureDirect(0xF, 0xE, 5, 8, 32);
    EXPECT_EQ(0xDF1E0001U, ts2.int1);
    EXPECT_EQ(0x05082000U, ts2.int2);
    EXPECT_EQ(0U, ts2.int3);
    EXPECT_EQ(0U, ts2.int4);
}

TEST_F(UmpFactoryTest, testMetronome) {
    auto metronome1 = UmpFactory::metronome(0, 0, 3, 4, 4, 1, 0, 0);
    EXPECT_EQ(0xD0100002U, metronome1.int1);
    EXPECT_EQ(0x03040401U, metronome1.int2);
    EXPECT_EQ(0U, metronome1.int3);
    EXPECT_EQ(0U, metronome1.int4);

    auto metronome2 = UmpFactory::metronome(0xF, 0xE, 2, 3, 2, 0, 2, 3);
    EXPECT_EQ(0xDF1E0002U, metronome2.int1);
    EXPECT_EQ(0x02030200U, metronome2.int2);
    EXPECT_EQ(0x02030000U, metronome2.int3);
    EXPECT_EQ(0U, metronome2.int4);
}

TEST_F(UmpFactoryTest, testKeySignature) {
    auto ks1 = UmpFactory::keySignature(0, 0, 0, 2, 6);
    EXPECT_EQ(0xD0000005U, ks1.int1);
    EXPECT_EQ(0x26000000U, ks1.int2);
    EXPECT_EQ(0U, ks1.int3);
    EXPECT_EQ(0U, ks1.int4);

    auto ks2 = UmpFactory::keySignature(0xF, 1, 0xE, -2, 7);
    EXPECT_EQ(0xDF1E0005U, ks2.int1);
    EXPECT_EQ(0xE7000000U, ks2.int2);
    EXPECT_EQ(0U, ks2.int3);
    EXPECT_EQ(0U, ks2.int4);
}

TEST_F(UmpFactoryTest, testChordName) {
    auto chordName1 = UmpFactory::chordName(0, 0, 0, 1, 6, 1, 0x11, 1, 2, 3, 1, 3, 1, 1, 2);
    EXPECT_EQ(0xD0000006U, chordName1.int1);
    EXPECT_EQ(0x16011101U, chordName1.int2);
    EXPECT_EQ(0x02030000U, chordName1.int3);
    EXPECT_EQ(0x13010102U, chordName1.int4);

    auto chordName2 = UmpFactory::chordName(0xF, 1, 0xE, -2, 7, 0x1B, 0x21, 0x21, 0x32, 3, -1, 3, 0x14, 0x30, 2);
    EXPECT_EQ(0xDF1E0006U, chordName2.int1);
    EXPECT_EQ(0xE71B2121U, chordName2.int2);
    EXPECT_EQ(0x32030000U, chordName2.int3);
    EXPECT_EQ(0xF3143002U, chordName2.int4);
}

TEST_F(UmpFactoryTest, testMetadataText) {
    auto text1 = UmpFactory::metadataText(0, 0, 0, 0, "TEST STRING");
    EXPECT_EQ(1, text1.size());
    EXPECT_EQ(0xD0000100U, text1[0].int1);
    EXPECT_EQ(0x54455354U, text1[0].int2);
    EXPECT_EQ(0x20535452U, text1[0].int3);
    EXPECT_EQ(0x494E4700U, text1[0].int4);

    auto text2 = UmpFactory::metadataText(0, 0, 0, 1, "TEST STRING1");
    EXPECT_EQ(1, text2.size());
    EXPECT_EQ(0xD0000101U, text2[0].int1);
    EXPECT_EQ(0x54455354U, text2[0].int2);
    EXPECT_EQ(0x20535452U, text2[0].int3);
    EXPECT_EQ(0x494E4731U, text2[0].int4);

    auto text3 = UmpFactory::metadataText(0, 0, 5, 0, "Test String That Spans More.");
    EXPECT_EQ(3, text3.size());
    EXPECT_EQ(0xD0450100U, text3[0].int1);
    EXPECT_EQ(0x54657374U, text3[0].int2);
    EXPECT_EQ(0x20537472U, text3[0].int3);
    EXPECT_EQ(0x696E6720U, text3[0].int4);
    EXPECT_EQ(0xD0850100U, text3[1].int1);
    EXPECT_EQ(0x54686174U, text3[1].int2);
    EXPECT_EQ(0x20537061U, text3[1].int3);
    EXPECT_EQ(0x6E73204DU, text3[1].int4);
    EXPECT_EQ(0xD0C50100U, text3[2].int1);
    EXPECT_EQ(0x6F72652EU, text3[2].int2);
    EXPECT_EQ(0U, text3[2].int3);
    EXPECT_EQ(0U, text3[2].int4);
}

TEST_F(UmpFactoryTest, testPerformanceText) {
    std::string lyrics = "A melisma";
    lyrics += '\0';
    lyrics += "ah";
    auto text1 = UmpFactory::performanceText(0, 0, 5, 1, lyrics);
    EXPECT_EQ(1, text1.size());
    EXPECT_EQ(0xD0050201U, text1[0].int1);
    EXPECT_EQ(0x41206D65U, text1[0].int2);
    EXPECT_EQ(0x6C69736DU, text1[0].int3);
    EXPECT_EQ(0x61006168U, text1[0].int4);
}

TEST_F(UmpFactoryTest, testEndpointDiscovery) {
    auto ed1 = UmpFactory::endpointDiscovery(1, 1, 0x1F);
    EXPECT_EQ(0xF0000101U, ed1.int1);
    EXPECT_EQ(0x1F, ed1.int2);
    EXPECT_EQ(0, ed1.int3);
    EXPECT_EQ(0, ed1.int4);
}

TEST_F(UmpFactoryTest, testEndpointInfoNotification) {
    auto en1 = UmpFactory::endpointInfoNotification(1, 1, true, 2, true, true, false, true);
    EXPECT_EQ(0xF0010101U, en1.int1);
    EXPECT_EQ(0x82000301U, en1.int2);
    EXPECT_EQ(0, en1.int3);
    EXPECT_EQ(0, en1.int4);
}

TEST_F(UmpFactoryTest, testDeviceIdentityNotification) {
    auto dn1 = UmpFactory::deviceIdentityNotification(0x123456, 0x789A, 0x7654, 0x32106543);
    EXPECT_EQ(0xF0020000U, dn1.int1);
    EXPECT_EQ(0x00123456, dn1.int2);
    EXPECT_EQ(0x789A7654, dn1.int3);
    EXPECT_EQ(0x32106543U, dn1.int4);
}

TEST_F(UmpFactoryTest, testEndpointNameNotification) {
    auto en1 = UmpFactory::endpointNameNotification("EndpointName12");
    EXPECT_EQ(1, en1.size());
    EXPECT_EQ(0xF003456EU, en1[0].int1);
    EXPECT_EQ(0x64706F69, en1[0].int2);
    EXPECT_EQ(0x6E744E61, en1[0].int3);
    EXPECT_EQ(0x6D653132, en1[0].int4);

    auto en2 = UmpFactory::endpointNameNotification("EndpointName123");
    EXPECT_EQ(2, en2.size());
    EXPECT_EQ(0xF403456EU, en2[0].int1);
    EXPECT_EQ(0xFC033300U, en2[1].int1);
    EXPECT_EQ(0, en2[1].int2);
    EXPECT_EQ(0, en2[1].int3);
    EXPECT_EQ(0, en2[1].int4);
}

TEST_F(UmpFactoryTest, testProductInstanceIdNotification) {
    auto pn1 = UmpFactory::productInstanceIdNotification("ProductName 123");
    EXPECT_EQ(2, pn1.size());
    EXPECT_EQ(0xF4045072U, pn1[0].int1);
    EXPECT_EQ(0xFC043300U, pn1[1].int1);
    EXPECT_EQ(0, pn1[1].int2);
    EXPECT_EQ(0, pn1[1].int3);
    EXPECT_EQ(0, pn1[1].int4);
}

TEST_F(UmpFactoryTest, testStreamConfigRequest) {
    auto req1 = UmpFactory::streamConfigRequest(3, true, false);
    EXPECT_EQ(0xF0050302U, req1.int1);
    EXPECT_EQ(0, req1.int2);
    EXPECT_EQ(0, req1.int3);
    EXPECT_EQ(0, req1.int4);
}

TEST_F(UmpFactoryTest, testStreamConfigNotification) {
    auto not1 = UmpFactory::streamConfigNotification(1, true, false);
    EXPECT_EQ(0xF0060102U, not1.int1);
    EXPECT_EQ(0, not1.int2);
    EXPECT_EQ(0, not1.int3);
    EXPECT_EQ(0, not1.int4);
}

TEST_F(UmpFactoryTest, testFunctionBlockDiscovery) {
    auto d1 = UmpFactory::functionBlockDiscovery(5, 3);
    EXPECT_EQ(0xF0100503U, d1.int1);
    EXPECT_EQ(0, d1.int2);
    EXPECT_EQ(0, d1.int3);
    EXPECT_EQ(0, d1.int4);
}

TEST_F(UmpFactoryTest, testFunctionBlockInfoNotification) {
    auto fb1 = UmpFactory::functionBlockInfoNotification(true, 5, 3, 2, 1, 0, 3, 1, 255);
    EXPECT_EQ(0xF0118539U, fb1.int1);
    EXPECT_EQ(0x000301FF, fb1.int2);
    EXPECT_EQ(0, fb1.int3);
    EXPECT_EQ(0, fb1.int4);
}

TEST_F(UmpFactoryTest, testFunctionBlockNameNotification) {
    auto fn1 = UmpFactory::functionBlockNameNotification(7, "FunctionName1");
    EXPECT_EQ(1, fn1.size());
    EXPECT_EQ(0xF0120746U, fn1[0].int1);
    EXPECT_EQ(0x756E6374, fn1[0].int2);
    EXPECT_EQ(0x696F6E4E, fn1[0].int3);
    EXPECT_EQ(0x616D6531, fn1[0].int4);

    auto fn2 = UmpFactory::functionBlockNameNotification(7, "FunctionName12");
    EXPECT_EQ(2, fn2.size());
    EXPECT_EQ(0xF4120746U, fn2[0].int1);
    EXPECT_EQ(0xFC120732U, fn2[1].int1);
    EXPECT_EQ(0, fn2[1].int2);
    EXPECT_EQ(0, fn2[1].int3);
    EXPECT_EQ(0, fn2[1].int4);
}