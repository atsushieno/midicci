#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace midicci::ump;

class UmpRetrieverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Any setup needed for UMP retriever tests
    }
};

TEST_F(UmpRetrieverTest, testGetSysex7Data1) {
    std::vector<uint8_t> src1 = {0, 0, 1, 2, 3, 4};
    std::vector<Ump> packets;
    
    UmpFactory::sysex7_process(0, src1, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });
    
    auto actual1 = UmpRetriever::get_sysex7_data(packets);
    ASSERT_EQ(src1.size(), actual1.size());
    for (size_t i = 0; i < src1.size(); ++i) {
        EXPECT_EQ(src1[i], actual1[i]) << "Mismatch at index " << i;
    }
}

TEST_F(UmpRetrieverTest, testGetSysex7Data2) {
    std::vector<uint8_t> src2 = {0, 0, 1, 2, 3, 4, 5};
    std::vector<Ump> packets;
    
    UmpFactory::sysex7_process(0, src2, [&packets](const Ump& ump) {
        packets.push_back(ump);
    });
    
    auto actual2 = UmpRetriever::get_sysex7_data(packets);
    ASSERT_EQ(src2.size(), actual2.size());
    for (size_t i = 0; i < src2.size(); ++i) {
        EXPECT_EQ(src2[i], actual2[i]) << "Mismatch at index " << i;
    }
}

TEST_F(UmpRetrieverTest, testGetSysex8Data) {
    std::vector<uint8_t> src1 = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    std::vector<Ump> packets;
    
    // Note: This test would require implementing sysex8_process in UmpFactory
    // For now, we'll skip this test
    GTEST_SKIP() << "Requires implementing UmpFactory::sysex8_process";
}

// Additional tests would require more UmpFactory and Ump methods to be implemented
TEST_F(UmpRetrieverTest, DISABLED_testNeedsMoreMethods) {
    // These tests would require implementing additional methods:
    // - UMP parsing for tempo, time signature, metronome, etc.
    // - String extraction for text messages
    // - Endpoint name/product ID extraction
    // - Function block name extraction
    
    GTEST_SKIP() << "Requires implementing more Ump accessor methods";
}