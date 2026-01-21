#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include "midicci/midicci.hpp"

using namespace midicci;

TEST(RequestIdCorrelationTest, GetPropertyDataReplyCorrelation) {
    // Create two devices - client and server
    MidiCIDeviceConfiguration config;
    MidiCIDeviceConfiguration server_config;
    
    auto client_device = std::make_shared<MidiCIDevice>(0x12345678, config);
    auto server_device = std::make_shared<MidiCIDevice>(0x87654321, server_config);
    
    // Test 1: Create GetPropertyData with specific requestId
    uint8_t test_request_id = 42;
    Common common(0x12345678, 0x87654321, ADDRESS_FUNCTION_BLOCK, 0);
    
    // Create a simple header for ResourceList
    std::string header_json = R"({"resource":"ResourceList"})";
    std::vector<uint8_t> header(header_json.begin(), header_json.end());
    
    GetPropertyData request(common, test_request_id, header);
    
    EXPECT_EQ(request.getRequestId(), test_request_id);
    
    // Test 2: Create GetPropertyDataReply with same requestId
    std::string reply_header_json = R"({"status":200})";
    std::vector<uint8_t> reply_header(reply_header_json.begin(), reply_header_json.end());
    std::string reply_body_json = R"([])";
    std::vector<uint8_t> reply_body(reply_body_json.begin(), reply_body_json.end());
    
    Common reply_common(0x87654321, 0x12345678, ADDRESS_FUNCTION_BLOCK, 0);
    GetPropertyDataReply reply(reply_common, test_request_id, reply_header, reply_body);
    
    EXPECT_EQ(reply.getRequestId(), test_request_id);
    
    // Test 3: Verify correlation matching
    EXPECT_EQ(request.getRequestId(), reply.getRequestId());
    
    // Test 4: Test with different requestIds to ensure they don't match
    GetPropertyDataReply different_reply(reply_common, 99, reply_header, reply_body);
    EXPECT_NE(request.getRequestId(), different_reply.getRequestId());
}

TEST(RequestIdCorrelationTest, PropertyClientFacadeSequence) {
    MidiCIDeviceConfiguration config;
    auto client_device = std::make_shared<MidiCIDevice>(0x12345678, config);
    
    auto connection = std::make_shared<ClientConnection>(*client_device, 0x87654321, DeviceDetails{0x123, 0x456, 0x789, 0xABC}, 4096);
    auto property_client = std::make_unique<PropertyClientFacade>(*client_device, *connection);
    
    // Get next request ID from messenger
    auto next_id1 = client_device->getMessenger().getNextRequestId();
    auto next_id2 = client_device->getMessenger().getNextRequestId();
    
    EXPECT_EQ(next_id2, next_id1 + 1);
}
