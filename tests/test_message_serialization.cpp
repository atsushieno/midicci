#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include "midicci/midicci.hpp"

using namespace midicci;

TEST(MessageSerializationTest, GetPropertyDataSerialization) {
    // Create a device
    MidiCIDeviceConfiguration config;
    auto device = std::make_shared<MidiCIDevice>(0x12345678, config);
    
    // Create GetPropertyData with specific request ID
    uint8_t test_request_id = 42;
    Common common(0x12345678, 0x87654321, ADDRESS_FUNCTION_BLOCK, 0);
    
    std::string header_json = R"({"resource":"ResourceList"})";
    std::vector<uint8_t> header(header_json.begin(), header_json.end());
    
    GetPropertyData request(common, test_request_id, header);
    
    // Original GetPropertyData requestId
    EXPECT_EQ(request.getRequestId(), test_request_id);
    
    // Serialize the message
    auto serialized = request.serialize(config);
    
    // Serialized message count
    EXPECT_FALSE(serialized.empty());
    
    const auto& msg_bytes = serialized[0];
    // Serialized size should be adequate
    EXPECT_GE(msg_bytes.size(), 14u);
    
    uint8_t serialized_request_id = msg_bytes[13];
    // Request ID in serialized message (byte 13)
    EXPECT_EQ(serialized_request_id, test_request_id);
}

TEST(MessageSerializationTest, GetPropertyDataReplySerialization) {
    MidiCIDeviceConfiguration config;
    uint8_t test_request_id = 42;
    
    // Test GetPropertyDataReply serialization
    std::string reply_header_json = R"({"status":200})";
    std::vector<uint8_t> reply_header(reply_header_json.begin(), reply_header_json.end());
    std::string reply_body_json = R"([])";
    std::vector<uint8_t> reply_body(reply_body_json.begin(), reply_body_json.end());
    
    Common reply_common(0x87654321, 0x12345678, ADDRESS_FUNCTION_BLOCK, 0);
    GetPropertyDataReply reply(reply_common, test_request_id, reply_header, reply_body);
    
    // Original GetPropertyDataReply requestId
    EXPECT_EQ(reply.getRequestId(), test_request_id);
    
    auto reply_serialized = reply.serialize(config);
    
    EXPECT_FALSE(reply_serialized.empty());
    
    const auto& reply_bytes = reply_serialized[0];
    // Reply serialized size should be adequate
    EXPECT_GE(reply_bytes.size(), 14u);
    
    uint8_t reply_serialized_request_id = reply_bytes[13];
    // Reply Request ID in serialized message (byte 13)
    EXPECT_EQ(reply_serialized_request_id, test_request_id);
}

TEST(MessageSerializationTest, MessageReconstruction) {
    MidiCIDeviceConfiguration config;
    uint8_t test_request_id = 42;
    
    // Create reply message
    std::string reply_header_json = R"({"status":200})";
    std::vector<uint8_t> reply_header(reply_header_json.begin(), reply_header_json.end());
    std::string reply_body_json = R"([])";
    std::vector<uint8_t> reply_body(reply_body_json.begin(), reply_body_json.end());
    
    Common reply_common(0x87654321, 0x12345678, ADDRESS_FUNCTION_BLOCK, 0);
    GetPropertyDataReply reply(reply_common, test_request_id, reply_header, reply_body);
    
    auto reply_serialized = reply.serialize(config);
    ASSERT_FALSE(reply_serialized.empty());
    
    // Test reconstruction using CIRetrieval functions (like Messenger::processInput does)
    const auto& data = reply_serialized[0];
    
    ASSERT_GE(data.size(), 21u);
    
    uint8_t extracted_request_id = data[13];
    // Extracted request ID from reply message
    EXPECT_EQ(extracted_request_id, test_request_id);
    
    // Use CIRetrieval methods exactly like Messenger::processInput does
    uint32_t source_muid = CIRetrieval::getSourceMuid(data);
    uint32_t dest_muid = CIRetrieval::getDestinationMuid(data);
    
    // Extracted source MUID and dest MUID should match original
    EXPECT_EQ(source_muid, 0x87654321u);
    EXPECT_EQ(dest_muid, 0x12345678u);
    
    // Extract header and body using CIRetrieval methods
    std::vector<uint8_t> extracted_header = CIRetrieval::getPropertyHeader(data);
    std::vector<uint8_t> extracted_body = CIRetrieval::getPropertyBodyInThisChunk(data);
    
    // Header and body sizes should be reasonable
    EXPECT_GT(extracted_header.size(), 0u);
    EXPECT_GE(extracted_body.size(), 0u);
    
    Common reconstructed_common(source_muid, dest_muid, ADDRESS_FUNCTION_BLOCK, 0);
    GetPropertyDataReply reconstructed(reconstructed_common, extracted_request_id, extracted_header, extracted_body);
    
    // Reconstructed reply requestId should match original
    EXPECT_EQ(reconstructed.getRequestId(), test_request_id);
}