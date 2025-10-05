#include <gtest/gtest.h>
#include "midicci/midicci.hpp"
#include "AllCtrlListTestData.hpp"
#include <cstring>

using namespace midicci;
using namespace midicci::commonproperties;

class AllCtrlListParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_unique<MidiCIDeviceConfiguration>();
        config->max_property_chunk_size = 512;  // Small chunk size to force multiple chunks
        config->receivable_max_sysex_size = 4096;
    }

    std::unique_ptr<MidiCIDeviceConfiguration> config;
};

TEST_F(AllCtrlListParserTest, ParseOPNPlugAEAllCtrlList) {
    // Convert const char* to vector<uint8_t>
    const char* json_str = test::ALL_CTRL_LIST_OPNPLUG_AE;
    std::vector<uint8_t> json_data(json_str, json_str + std::strlen(json_str));

    // Parse the control list
    auto controls = StandardProperties::parseControlList(json_data);

    // Verify parsing succeeded
    EXPECT_FALSE(controls.empty());

    // Check a few known controls from the data
    bool found_master_volume = false;
    bool found_emulator = false;

    for (const auto& ctrl : controls) {
        if (ctrl.title == "Master volume") {
            found_master_volume = true;
            EXPECT_EQ(ctrl.ctrlType, "nrpn");
            EXPECT_EQ(ctrl.ctrlIndex.size(), 2);
            EXPECT_EQ(ctrl.ctrlIndex[0], 0);
            EXPECT_EQ(ctrl.ctrlIndex[1], 49);
        }
        if (ctrl.title == "Emulator") {
            found_emulator = true;
            EXPECT_EQ(ctrl.ctrlType, "nrpn");
            EXPECT_EQ(ctrl.ctrlIndex.size(), 2);
            EXPECT_EQ(ctrl.ctrlIndex[0], 0);
            EXPECT_EQ(ctrl.ctrlIndex[1], 21);
        }
    }

    EXPECT_TRUE(found_master_volume);
    EXPECT_TRUE(found_emulator);
}

TEST_F(AllCtrlListParserTest, SerializeAndVerifyChunkSequence) {
    // Convert const char* to vector<uint8_t>
    const char* json_str = test::ALL_CTRL_LIST_OPNPLUG_AE;
    std::vector<uint8_t> json_data(json_str, json_str + std::strlen(json_str));

    // Parse the control list
    auto controls = StandardProperties::parseControlList(json_data);
    ASSERT_FALSE(controls.empty());

    // Create a header for GetPropertyDataReply
    std::string header_json = R"({"resource":"AllCtrlList"})";
    std::vector<uint8_t> header(header_json.begin(), header_json.end());

    // Create GetPropertyDataReply message
    Common common(0x12345678, 0x87654321, 0, 0);
    GetPropertyDataReply reply(common, 1, header, json_data);

    // Serialize to chunks
    auto chunks = reply.serialize(*config);

    // Verify we got multiple chunks (the JSON is large)
    EXPECT_GT(chunks.size(), 1) << "Expected multiple chunks due to large JSON data";

    // Extract and verify chunk numbers are sequential
    // The structure is:
    // 0-12: MIDI CI message header
    // 13: request_id
    // 14-15: header size (7-bit encoded 14-bit value)
    // 16 to (16+header_size): header data
    // Then at offset (16+header_size):
    //   +0 to +1: num_chunks (7-bit encoded)
    //   +2 to +3: chunk_index (7-bit encoded)
    //   +4 to +5: chunk_data_size (7-bit encoded)

    uint16_t expected_num_chunks = chunks.size();

    for (size_t i = 0; i < chunks.size(); ++i) {
        const auto& chunk = chunks[i];

        // Ensure chunk is large enough to contain the header size
        ASSERT_GE(chunk.size(), 16) << "Chunk " << i << " is too small";

        // Extract header size (7-bit encoded at offset 14-15)
        uint16_t header_size = chunk[14] | (static_cast<uint16_t>(chunk[15] & 0x7F) << 7);

        // Calculate offset where chunk metadata starts
        size_t chunk_metadata_offset = 16 + header_size;

        // Ensure chunk is large enough to contain chunk metadata
        ASSERT_GE(chunk.size(), chunk_metadata_offset + 6) << "Chunk " << i << " is too small for metadata";

        // Extract num_chunks (7-bit encoded)
        uint16_t num_chunks = chunk[chunk_metadata_offset] |
                               (static_cast<uint16_t>(chunk[chunk_metadata_offset + 1] & 0x7F) << 7);

        // Extract chunk_index (7-bit encoded)
        uint16_t chunk_index = chunk[chunk_metadata_offset + 2] |
                                (static_cast<uint16_t>(chunk[chunk_metadata_offset + 3] & 0x7F) << 7);

        // Verify chunk index is sequential (1-based)
        EXPECT_EQ(chunk_index, i + 1) << "Chunk index at position " << i << " is not sequential";

        // Verify num_chunks is consistent across all chunks
        EXPECT_EQ(num_chunks, expected_num_chunks) << "Num chunks mismatch in chunk " << i;
    }
}

TEST_F(AllCtrlListParserTest, RoundTripConversion) {
    // Convert const char* to vector<uint8_t>
    const char* json_str = test::ALL_CTRL_LIST_OPNPLUG_AE;
    std::vector<uint8_t> json_data(json_str, json_str + std::strlen(json_str));

    // Parse the control list
    auto controls = StandardProperties::parseControlList(json_data);
    ASSERT_FALSE(controls.empty());

    // Serialize back to JSON
    auto regenerated_json = StandardProperties::toJson(controls);

    // Parse again
    auto reparsed_controls = StandardProperties::parseControlList(regenerated_json);

    // Verify we get the same number of controls
    EXPECT_EQ(controls.size(), reparsed_controls.size());

    // Verify a sample of controls match
    if (controls.size() > 0 && reparsed_controls.size() > 0) {
        EXPECT_EQ(controls[0].title, reparsed_controls[0].title);
        EXPECT_EQ(controls[0].ctrlType, reparsed_controls[0].ctrlType);
        EXPECT_EQ(controls[0].ctrlIndex, reparsed_controls[0].ctrlIndex);
    }
}
