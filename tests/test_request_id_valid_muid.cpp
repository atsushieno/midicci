#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci;

class RequestIdValidMuidTest : public ::testing::Test {
protected:
    std::vector<std::vector<uint8_t>> sent_messages;
    
    bool mock_send_output(uint8_t group, const std::vector<uint8_t>& data) {
        sent_messages.push_back(data);
        return true;
    }
    
    void SetUp() override {
        sent_messages.clear();
    }
};

TEST_F(RequestIdValidMuidTest, RequestIdCorrelationWithValidMUIDs) {
    // Testing requestId correlation with valid MIDI-CI MUIDs
        
        // Use valid MIDI-CI MUIDs (28-bit values, no byte > 0x7F)
        uint32_t client_muid = 0x12345678;   // Invalid: 0x78 > 0x7F
        uint32_t server_muid = 0x07654321;   // Valid: all bytes <= 0x7F
        
        // Actually, let's use completely valid MUIDs
        client_muid = 0x12345670;  // Valid: 0x70 <= 0x7F
        server_muid = 0x07654321;  // Valid: all bytes <= 0x7F
        
        // Using client MUID and server MUID with valid values
        
        // Create devices
        MidiCIDeviceConfiguration config;
        auto client_device = std::make_shared<MidiCIDevice>(client_muid, config);
        auto server_device = std::make_shared<MidiCIDevice>(server_muid, config);
        
        // Set up mock output handlers
        client_device->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) {
            return this->mock_send_output(group, data);
        });
        server_device->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) {
            return this->mock_send_output(group, data);
        });
        
        // Created client and server devices
        
        // Create connection and property client
        auto connection = std::make_shared<ClientConnection>(*client_device, server_muid, 
            DeviceDetails{0x123, 0x456, 0x789, 0xABC});
        auto property_client = std::make_unique<PropertyClientFacade>(*client_device, *connection);
        
        // Created PropertyClientFacade
        
        // Send a property request
        sent_messages.clear();
        property_client->send_get_property_data("ResourceList", "", "UTF-8", -1, -1);
        // Sent ResourceList request
        
        ASSERT_FALSE(sent_messages.empty()) << "Messages should have been sent";
        const auto& first_msg = sent_messages[0];
        ASSERT_GE(first_msg.size(), 14) << "Message should be long enough";
        
        // Extract and test MUID encoding/decoding
        
        // Extract 28-bit encoded MUIDs
        uint32_t source_28bit = first_msg[5] | (first_msg[6] << 7) | (first_msg[7] << 14) | (first_msg[8] << 21);
        uint32_t dest_28bit = first_msg[9] | (first_msg[10] << 7) | (first_msg[11] << 14) | (first_msg[12] << 21);
        
        // Test 28-bit encoded values are reasonable
        EXPECT_GT(source_28bit, 0) << "Source 28-bit encoding should be non-zero";
        EXPECT_GT(dest_28bit, 0) << "Dest 28-bit encoding should be non-zero";
        
        // Convert back to 32-bit
        uint32_t reconstructed_source = ((source_28bit >> 21) << 24) | 
                                      (((source_28bit >> 14) & 0x7F) << 16) |
                                      (((source_28bit >> 7) & 0x7F) << 8) |
                                      (source_28bit & 0x7F);
        
        uint32_t reconstructed_dest = ((dest_28bit >> 21) << 24) | 
                                    (((dest_28bit >> 14) & 0x7F) << 16) |
                                    (((dest_28bit >> 7) & 0x7F) << 8) |
                                    (dest_28bit & 0x7F);
        
        // Check if reconstruction is working correctly
        
        // Test the MUID correlation (note: may have encoding differences but should be consistent)
        EXPECT_EQ(reconstructed_source, client_muid) << "Source MUID should be correctly encoded/decoded";
        EXPECT_EQ(reconstructed_dest, server_muid) << "Dest MUID should be correctly encoded/decoded";
        
        // Test requestId extraction
        uint8_t extracted_request_id = first_msg[13];
        EXPECT_GT(extracted_request_id, 0) << "Request ID should be non-zero";
        
        // Create a reply and test correlation
        std::string reply_header = R"({"status": 200})";
        std::string reply_body = R"([])";
        std::vector<uint8_t> header_bytes(reply_header.begin(), reply_header.end());
        std::vector<uint8_t> body_bytes(reply_body.begin(), reply_body.end());
        
        Common reply_common(server_muid, client_muid, ADDRESS_FUNCTION_BLOCK, 0);
        GetPropertyDataReply reply(reply_common, extracted_request_id, header_bytes, body_bytes);
        
        EXPECT_EQ(extracted_request_id, reply.get_request_id()) << "Reply should have correct request ID";
        
        // Process the reply to test the full round-trip
        EXPECT_NO_THROW(property_client->process_get_data_reply(reply)) << "Reply processing should not throw";
}