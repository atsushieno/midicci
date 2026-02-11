#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace umppi;

class UmpTranslatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed for UmpTranslator tests
    }
};

TEST_F(UmpTranslatorTest, testConvertSingleUmpToMidi1) {
    std::vector<uint8_t> dst(16, 0);

    // MIDI1 Channel Voice Messages

    auto ump = Ump(UmpFactory::midi1NoteOff(0, 1, 40, 0x70));
    auto size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    std::vector<uint8_t> expected = {0x81, 40, 0x70};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    ump = Ump(UmpFactory::midi1Program(0, 1, 40));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(2, size);
    expected = {0xC1, 40};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // MIDI2 Channel Voice Messages

    // RPN
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2RPN(0, 1, 2, 3, 517 * 0x40000))); // MIDI1 DTE 517, expanded to 32bit
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(12, size);
    // 4 = 517 / 0x80, 5 = 517 % 0x80
    expected = {0xB1, 101, 0x2, 0xB1, 100, 0x3, 0xB1, 6, 4, 0xB1, 38, 5};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // NRPN
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2NRPN(0, 1, 2, 3, 0xFF000000)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(12, size);
    expected = {0xB1, 99, 0x2, 0xB1, 98, 0x3, 0xB1, 6, 0x7F, 0xB1, 38, 0x40};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // Note off
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2NoteOff(0, 1, 40, 0, 0xE800, 0)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    expected = {0x81, 40, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // Note on
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2NoteOn(0, 1, 40, 0, 0xE800, 0)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    expected = {0x91, 40, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // PAf
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2PAf(0, 1, 40, 0xE8000000)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    expected = {0xA1, 40, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // CC
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2CC(0, 1, 10, 0xE8000000)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    expected = {0xB1, 10, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // Program change, without bank options
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2Program(0, 1, 0, 8, 16, 24)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(2, size);
    expected = {0xC1, 8};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // Program change, with bank options
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2Program(0, 1, 1, 8, 16, 24)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(8, size);
    expected = {0xB1, 0, 16, 0xB1, 32, 24, 0xC1, 8};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // CAf
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2CAf(0, 1, 0xE8000000)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(2, size);
    expected = {0xD1, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }

    // Pitch Bend
    ump = Ump(static_cast<uint64_t>(UmpFactory::midi2PitchBendDirect(0, 1, 0xE8040000)));
    size = UmpTranslator::translateSingleUmpToMidi1Bytes(dst, ump);
    EXPECT_EQ(3, size);
    expected = {0xE1, 1, 0x74};
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(expected[i], dst[i]) << "Mismatch at index " << i;
    }
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpNoteOn) {
    std::vector<uint8_t> bytes = {0x91, 0x40, 0x78};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47914000, context.output[0].int1);
    EXPECT_EQ(0xF0000000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpWithSmfDeltaTime) {
    std::vector<uint8_t> bytes = {0x02, 0x91, 0x40, 0x78};
    Midi1ToUmpTranslatorContext context(bytes, 7, false,
                                        static_cast<int>(MidiTransportProtocol::UMP),
                                        false, true);

    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(4, context.midi1Pos);
    ASSERT_EQ(2, context.output.size());
    EXPECT_TRUE(context.output[0].isDeltaClockstamp());
    EXPECT_EQ(UmpFactory::deltaClockstamp(2), context.output[0].int1);
    EXPECT_EQ(0x47914000, context.output[1].int1);
    EXPECT_EQ(0xF0000000U, context.output[1].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpSmfTempoMeta) {
    std::vector<uint8_t> bytes = {0x00, 0xFF, MidiMetaType::TEMPO, 0x03, 0x07, 0xA1, 0x20,
                                  0x00, 0x91, 0x40, 0x60};
    Midi1ToUmpTranslatorContext context(bytes, 0, false,
                                        static_cast<int>(MidiTransportProtocol::UMP),
                                        false, true);

    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    ASSERT_EQ(2, context.output.size());
    EXPECT_EQ(umppi::MessageType::FLEX_DATA, context.output[0].getMessageType());
    EXPECT_EQ(0xD0100000U, context.output[0].int1);
    EXPECT_EQ(0x02FAF080U, context.output[0].int2);
    EXPECT_EQ(umppi::MessageType::MIDI2, context.output[1].getMessageType());
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpSmfTimeSignatureMeta) {
    std::vector<uint8_t> bytes = {0x00, 0xFF, MidiMetaType::TIME_SIGNATURE, 0x04, 0x03, 0x02, 0x18, 0x08,
                                  0x00, 0x91, 0x40, 0x60};
    Midi1ToUmpTranslatorContext context(bytes, 0, false,
                                        static_cast<int>(MidiTransportProtocol::UMP),
                                        false, true);

    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    ASSERT_EQ(2, context.output.size());
    EXPECT_EQ(umppi::MessageType::FLEX_DATA, context.output[0].getMessageType());
    EXPECT_EQ(0xD0100001U, context.output[0].int1);
    EXPECT_EQ(0x03040800U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpSmfLyricMeta) {
    std::vector<uint8_t> bytes = {0x00, 0xFF, MidiMetaType::LYRIC, 0x02, 'H', 'i',
                                  0x00, 0x91, 0x40, 0x60};
    Midi1ToUmpTranslatorContext context(bytes, 0, false,
                                        static_cast<int>(MidiTransportProtocol::UMP),
                                        false, true);

    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    ASSERT_EQ(2, context.output.size());
    EXPECT_EQ(0xD0100201U, context.output[0].int1);
    EXPECT_EQ(0x48690000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpPAf) {
    std::vector<uint8_t> bytes = {0xA1, 0x40, 0x60};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // PAf
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47A14000, context.output[0].int1);
    EXPECT_EQ(0xC0000000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpSimpleCC) {
    std::vector<uint8_t> bytes = {0xB1, 0x07, 0x70};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Simple CC
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47B10700, context.output[0].int1);
    EXPECT_EQ(0xE0000000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpValidRPN) {
    std::vector<uint8_t> bytes = {0xB1, 101, 1, 0xB1, 100, 2, 0xB1, 6, 0x10, 0xB1, 38, 0x20};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // RPN
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(12, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47210102, context.output[0].int1);
    EXPECT_EQ(0x20800000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpValidNRPN) {
    std::vector<uint8_t> bytes = {0xB1, 99, 1, 0xB1, 98, 2, 0xB1, 6, 0x10, 0xB1, 38, 0x20};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // NRPN
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(12, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47310102, context.output[0].int1);
    EXPECT_EQ(0x20800000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpInvalidRPN) {
    // Only RPN MSB -> error
    std::vector<uint8_t> bytes = {0xB1, 101, 1};
    Midi1ToUmpTranslatorContext context(bytes, 7);
    
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());

    // Only RPN MSB and LSB -> error
    bytes = {0xB1, 101, 1, 0xB1, 100, 2};
    context = Midi1ToUmpTranslatorContext(bytes, 7);
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(6, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());

    // Only RPN MSB and LSB, and DTE MSB -> error
    bytes = {0xB1, 101, 1, 0xB1, 100, 2, 0xB1, 6, 3};
    context = Midi1ToUmpTranslatorContext(bytes, 7);
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(9, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpInvalidNRPN) {
    // Only NRPN MSB -> error
    std::vector<uint8_t> bytes = {0xB1, 99, 1};
    Midi1ToUmpTranslatorContext context(bytes, 7);
    
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());

    // Only NRPN MSB and LSB -> error
    bytes = {0xB1, 99, 1, 0xB1, 98, 2};
    context = Midi1ToUmpTranslatorContext(bytes, 7);
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(6, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());

    // Only NRPN MSB and LSB, and DTE MSB -> error
    bytes = {0xB1, 99, 1, 0xB1, 98, 2, 0xB1, 6, 3};
    context = Midi1ToUmpTranslatorContext(bytes, 7);
    EXPECT_EQ(UmpTranslationResult::INVALID_DTE_SEQUENCE, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(9, context.midi1Pos);
    EXPECT_EQ(0, context.output.size());
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpSimpleProgramChange) {
    std::vector<uint8_t> bytes = {0xC1, 0x30};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Simple program change
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(2, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47C10000, context.output[0].int1);
    EXPECT_EQ(0x30000000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpBankMsbLsbAndProgramChange) {
    std::vector<uint8_t> bytes = {0xB1, 0x00, 0x12, 0xB1, 0x20, 0x22, 0xC1, 0x30};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Bank select MSB, bank select LSB, program change
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(8, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47C10001, context.output[0].int1);
    EXPECT_EQ(0x30001222U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpBankMsbAndProgramChange) {
    std::vector<uint8_t> bytes = {0xB1, 0x00, 0x12, 0xC1, 0x30};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Bank select MSB, then program change (LSB skipped)
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(5, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47C10001, context.output[0].int1);
    EXPECT_EQ(0x30001200U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpBankLsbAndProgramChange) {
    std::vector<uint8_t> bytes = {0xB1, 0x20, 0x12, 0xC1, 0x30};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Bank select LSB, then program change (MSB skipped)
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(5, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47C10001, context.output[0].int1);
    EXPECT_EQ(0x30000012U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpCAf) {
    std::vector<uint8_t> bytes = {0xD1, 0x60};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // CAf
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(2, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47D10000, context.output[0].int1);
    EXPECT_EQ(0xC0000000U, context.output[0].int2);
}

TEST_F(UmpTranslatorTest, testConvertMidi1ToUmpPitchBend) {
    std::vector<uint8_t> bytes = {0xE1, 0x20, 0x30};
    Midi1ToUmpTranslatorContext context(bytes, 7);

    // Pitch bend
    EXPECT_EQ(UmpTranslationResult::OK, UmpTranslator::translateMidi1BytesToUmp(context));
    EXPECT_EQ(3, context.midi1Pos);
    EXPECT_EQ(1, context.output.size());
    EXPECT_EQ(0x47E10000, context.output[0].int1);
    EXPECT_EQ(0x60800000U, context.output[0].int2); // Note that source MIDI1 pitch bend is in little endian.
}

TEST_F(UmpTranslatorTest, testRoundtripNoteMessages) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1NoteOn(0, 5, 60, 100)));
    midi1Umps.push_back(Ump(UmpFactory::midi1NoteOff(0, 5, 60, 64)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(2, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[1].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(2, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
    EXPECT_EQ(midi1Umps[1].int1, roundtripMidi1Umps[1].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripPAfMessage) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1PAf(0, 3, 60, 75)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(1, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(1, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripCCMessage) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1CC(0, 2, 7, 100)));
    midi1Umps.push_back(Ump(UmpFactory::midi1CC(0, 2, 10, 64)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(2, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[1].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(2, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
    EXPECT_EQ(midi1Umps[1].int1, roundtripMidi1Umps[1].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripProgramChangeMessage) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1Program(0, 4, 42)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(1, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(1, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripCAfMessage) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1CAf(0, 6, 80)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(1, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(1, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripPitchBendMessage) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1PitchBendDirect(0, 7, 0x2040)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(1, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[0].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(1, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
}

TEST_F(UmpTranslatorTest, testRoundtripMixedMessages) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::midi1NoteOn(0, 1, 60, 100)));
    midi1Umps.push_back(Ump(UmpFactory::midi1CC(0, 1, 7, 127)));
    midi1Umps.push_back(Ump(UmpFactory::midi1PitchBendDirect(0, 1, 0x2000)));
    midi1Umps.push_back(Ump(UmpFactory::midi1NoteOff(0, 1, 60, 64)));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(4, midi2Umps.size());
    for (const auto& ump : midi2Umps) {
        EXPECT_EQ(umppi::MessageType::MIDI2, ump.getMessageType());
    }

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(4, roundtripMidi1Umps.size());

    for (size_t i = 0; i < midi1Umps.size(); ++i) {
        EXPECT_EQ(midi1Umps[i].int1, roundtripMidi1Umps[i].int1)
            << "Mismatch at message index " << i;
    }
}

TEST_F(UmpTranslatorTest, testRoundtripPreservesNonMidi1Messages) {
    std::vector<Ump> midi1Umps;
    std::vector<Ump> midi2Umps;
    std::vector<Ump> roundtripMidi1Umps;

    midi1Umps.push_back(Ump(UmpFactory::deltaClockstamp(100)));
    midi1Umps.push_back(Ump(UmpFactory::midi1NoteOn(0, 1, 60, 100)));
    midi1Umps.push_back(Ump(UmpFactory::noop()));

    UmpTranslator::translateMidi1UmpToMidi2Ump(midi2Umps, midi1Umps);
    ASSERT_EQ(3, midi2Umps.size());
    EXPECT_EQ(umppi::MessageType::UTILITY, midi2Umps[0].getMessageType());
    EXPECT_EQ(umppi::MessageType::MIDI2, midi2Umps[1].getMessageType());
    EXPECT_EQ(umppi::MessageType::UTILITY, midi2Umps[2].getMessageType());

    UmpTranslator::translateMidi2UmpToMidi1Ump(roundtripMidi1Umps, midi2Umps);
    ASSERT_EQ(3, roundtripMidi1Umps.size());

    EXPECT_EQ(midi1Umps[0].int1, roundtripMidi1Umps[0].int1);
    EXPECT_EQ(umppi::MessageType::MIDI1, roundtripMidi1Umps[1].getMessageType());
    EXPECT_EQ(midi1Umps[2].int1, roundtripMidi1Umps[2].int1);
}
