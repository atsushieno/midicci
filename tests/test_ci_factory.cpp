#include <gtest/gtest.h>
#include "midicci/core/MidiCIConstants.hpp"
#include "midicci/core/CIFactory.hpp"
#include "midicci/messages/Message.hpp"
#include "midicci/profiles/MidiCIProfile.hpp"
#include <vector>
#include <cstdint>

using namespace midicci;
using namespace midicci::profilecommonrules;

TEST(CIFactoryTest, testDiscoveryMessages) {
    uint8_t all_supported = 0x1C;
    
    std::vector<uint8_t> expected1 = {
        0x7E, 0x7F, 0x0D, 0x70, 2,
        0x10, 0x10, 0x10, 0x10, 0x7F, 0x7F, 0x7F, 0x7F,
        0x56, 0x34, 0x12, 0x57, 0x13, 0x68, 0x24,
        0x7F, 0x5F, 0x3F, 0x1F,
        0x1C,
        0x00, 0x02, 0, 0, 0
    };
    std::vector<uint8_t> actual1(30, 0);
    auto result1 = CIFactory::midiCIDiscovery(
        actual1, 0x10101010,
        0x123456, 0x1357, 0x2468, 0x1F3F5F7F,
        all_supported,
        512,
        0
    );
    EXPECT_EQ(expected1, result1);

    std::vector<uint8_t> actual2(31, 0);
    CIFactory::midiCIDiscoveryReply(
        actual2, 1, 0x10101010, 0x20202020,
        0x123456, 0x1357, 0x2468, 0x1F3F5F7F,
        all_supported,
        512,
        0,
        0
    );
    EXPECT_EQ(0x71, actual2[3]);
    for (int i = 9; i <= 12; i++)
        EXPECT_EQ(0x20, actual2[i]);

    std::vector<uint8_t> expected3 = {
        0x7E, 0x7F, 0x0D, 0x7E, 1,
        0x10, 0x10, 0x10, 0x10, 0x7F, 0x7F, 0x7F, 0x7F, 0x20, 0x20, 0x20, 0x20
    };
    std::vector<uint8_t> actual3(17, 0);
    CIFactory::midiCIInvalidateMuid(actual3, 1, 0x10101010, 0x20202020);
    EXPECT_EQ(expected3, actual3);

    std::vector<uint8_t> expected4 = {
        0x7E, 5, 0x0D, 0x7F, 1,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20
    };
    std::vector<uint8_t> actual4(13, 0);
    CIFactory::midiCIDiscoveryNak(actual4, 5, 1, 0x10101010, 0x20202020);
    EXPECT_EQ(expected4, actual4);
}

TEST(CIFactoryTest, testProfileConfigurationMessages) {
    std::vector<uint8_t> expected1 = {
        0x7E, 5, 0x0D, 0x20, 2,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20
    };
    std::vector<uint8_t> actual1(13, 0);
    CIFactory::midiCIProfileInquiry(actual1, 5, 0x10101010, 0x20202020);
    EXPECT_EQ(expected1, actual1);

    uint8_t b7E = 0x7E;
    std::vector<MidiCIProfileId> profiles1 = {
        MidiCIProfileId({b7E, 2, 3, 4, 5}), 
        MidiCIProfileId({b7E, 7, 8, 9, 10})
    };
    std::vector<MidiCIProfileId> profiles2 = {
        MidiCIProfileId({b7E, 12, 13, 14, 15}), 
        MidiCIProfileId({b7E, 17, 18, 19, 20})
    };
    std::vector<uint8_t> expected2 = {
        0x7E, 5, 0x0D, 0x21, 2,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20,
        2,
        0,
        b7E, 2, 3, 4, 5,
        b7E, 7, 8, 9, 10,
        2,
        0,
        b7E, 12, 13, 14, 15,
        b7E, 17, 18, 19, 20
    };
    std::vector<uint8_t> actual2(37, 0);
    CIFactory::midiCIProfileInquiryReply(
        actual2, 5, 0x10101010, 0x20202020, profiles1, profiles2);
    EXPECT_EQ(expected2, actual2);

    std::vector<uint8_t> expected3 = {
        0x7E, 5, 0x0D, 0x22, 2,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20,
        0x7E, 2, 3, 4, 5, 1, 0
    };
    std::vector<uint8_t> actual3(20, 0);
    CIFactory::midiCIProfileSet(
        actual3, 5, true, 0x10101010, 0x20202020,
        profiles1[0], 1
    );
    EXPECT_EQ(expected3, actual3);
    EXPECT_EQ(1, actual3[18]);

    std::vector<uint8_t> actual4(20, 0);
    CIFactory::midiCIProfileSet(
        actual4, 5, false, 0x10101010, 0x20202020,
        profiles1[0], 1
    );
    EXPECT_EQ(0x23, actual4[3]);

    std::vector<uint8_t> expected5 = {
        0x7E, 5, 0x0D, 0x24, 2,
        0x10, 0x10, 0x10, 0x10, 0x7F, 0x7F, 0x7F, 0x7F,
        0x7E, 2, 3, 4, 5, 1, 0,
    };
    std::vector<uint8_t> actual5(20, 0);
    CIFactory::midiCIProfileReport(
        actual5, 5, true, 0x10101010,
        profiles1[0], 1
    );
    EXPECT_EQ(expected5, actual5);

    std::vector<uint8_t> expected6 = expected5;
    expected6[3] = 0x25;
    std::vector<uint8_t> actual6(20, 0);
    CIFactory::midiCIProfileReport(
        actual6, 5, false, 0x10101010,
        profiles1[0], 1
    );
    EXPECT_EQ(expected6, actual6);

    std::vector<uint8_t> expected7 = {
        0x7E, 5, 0x0D, 0x2F, 2,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20,
        0x7E, 2, 3, 4, 5,
        8, 0, 0, 0,
        8, 7, 6, 5, 4, 3, 2, 1
    };
    std::vector<uint8_t> actual7(30, 0);
    std::vector<uint8_t> data = {8, 7, 6, 5, 4, 3, 2, 1};
    CIFactory::midiCIProfileSpecificData(
        actual7, 5, 0x10101010, 0x20202020,
        profiles1[0], data
    );
    EXPECT_EQ(expected7, actual7);
}

TEST(CIFactoryTest, testPropertyExchangeMessages) {
    std::vector<uint8_t> header = {11, 22, 33, 44};
    std::vector<uint8_t> data = {55, 66, 77, 88, 99};

    std::vector<uint8_t> expected1 = {
        0x7E, 5, 0x0D, 0x30, 2,
        0x10, 0x10, 0x10, 0x10, 0x20, 0x20, 0x20, 0x20,
        16, 0, 0
    };
    std::vector<uint8_t> actual1(16, 0);
    CIFactory::midiCIPropertyGetCapabilities(actual1, 5, false, 0x10101010, 0x20202020, 16);
    EXPECT_EQ(expected1, actual1);

    std::vector<uint8_t> actual2(16, 0);
    CIFactory::midiCIPropertyGetCapabilities(actual2, 5, true, 0x10101010, 0x20202020, 16);
    EXPECT_EQ(0x31, actual2[3]);

    std::vector<uint8_t> actual5(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual5, 5, static_cast<uint8_t>(CISubId2::PROPERTY_GET_DATA_INQUIRY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x34, actual5[3]);

    std::vector<uint8_t> actual6(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual6, 5, static_cast<uint8_t>(CISubId2::PROPERTY_GET_DATA_REPLY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x35, actual6[3]);

    std::vector<uint8_t> actual7(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual7, 5, static_cast<uint8_t>(CISubId2::PROPERTY_SET_DATA_INQUIRY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x36, actual7[3]);

    std::vector<uint8_t> actual8(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual8, 5, static_cast<uint8_t>(CISubId2::PROPERTY_SET_DATA_REPLY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x37, actual8[3]);

    std::vector<uint8_t> actual9(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual9, 5, static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x38, actual9[3]);

    std::vector<uint8_t> actual10(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual10, 5, static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_REPLY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x39, actual10[3]);

    std::vector<uint8_t> actual11(31, 0);
    CIFactory::midiCIPropertyCommon(
        actual11, 5, static_cast<uint8_t>(CISubId2::PROPERTY_NOTIFY),
        0x10101010, 0x20202020,
        2, header, 3, 1, data
    );
    EXPECT_EQ(0x3F, actual11[3]);
}

TEST(CIFactoryTest, midiCI32to28) {
    EXPECT_EQ(0xFFFFFFF, CIFactory::midiCI32to28(0x7F7F7F7F));
    EXPECT_EQ(0xFC285E9, CIFactory::midiCI32to28(0x7E0A0B69));
    EXPECT_EQ(0xCBD8657, CIFactory::midiCI32to28(0x65760C57));
}
