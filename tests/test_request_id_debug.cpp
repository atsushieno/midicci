#include <gtest/gtest.h>
#include <midicci/midicci.hpp>

using namespace midicci::commonproperties;
using namespace midicci;

class RequestIdDebugTest : public ::testing::Test {
protected:
    std::vector<std::string> log_messages;
    std::vector<uint8_t> client_to_server_data;
    std::vector<uint8_t> server_to_client_data;
    
    void debug_logger(const std::string& message, bool is_outgoing) {
        std::string direction = is_outgoing ? "OUTGOING" : "INCOMING";
        std::string log_entry = "[" + direction + "] " + message;
        log_messages.push_back(log_entry);
    }
    
    bool mock_transport_server_to_client(uint8_t group, const std::vector<uint8_t>& data) {
        server_to_client_data = data;
        return true;
    }
    
    bool mock_transport_client_to_server(uint8_t group, const std::vector<uint8_t>& data) {
        client_to_server_data = data;
        return true;
    }
    
    void SetUp() override {
        log_messages.clear();
        client_to_server_data.clear();
        server_to_client_data.clear();
    }
};

TEST_F(RequestIdDebugTest, RequestIdCorrelationWithDebugLogging) {
    // Testing Request ID Correlation with Debug Logging
        
        // Create client and server devices with debug logging
        MidiCIDeviceConfiguration client_config;
        MidiCIDeviceConfiguration server_config;
        
        auto client_device = std::make_shared<MidiCIDevice>(0x12345678, client_config);
        auto server_device = std::make_shared<MidiCIDevice>(0x87654321, server_config);
        
        // Set up debug loggers
        client_device->set_logger([this](const LogData& log_data) {
            if (log_data.has_message()) {
                this->debug_logger(log_data.get_message().get_log_message(), log_data.is_outgoing);
            } else {
                this->debug_logger(log_data.get_string(), log_data.is_outgoing);
            }
        });
        server_device->set_logger([this](const LogData& log_data) {
            if (log_data.has_message()) {
                this->debug_logger(log_data.get_message().get_log_message(), log_data.is_outgoing);
            } else {
                this->debug_logger(log_data.get_string(), log_data.is_outgoing);
            }
        });
        
        // Set up mock transports
        client_device->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) {
            return this->mock_transport_client_to_server(group, data);
        });
        server_device->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) {
            return this->mock_transport_server_to_client(group, data);
        });
        
        // Add some test properties to the server
        auto test_property = std::make_unique<CommonRulesPropertyMetadata>("TestProperty");
        test_property->canGet = true;
        test_property->canSet = false;
        server_device->get_property_host_facade().addMetadata(std::move(test_property));
        
        // Created client device (MUID: 0x12345678) and server device (MUID: 0x87654321)
        
        // Create a connection and property client
        auto connection = std::make_shared<ClientConnection>(*client_device, 0x87654321, 
            DeviceDetails{0x123, 0x456, 0x789, 0xABC});
        auto property_client = std::make_unique<PropertyClientFacade>(*client_device, *connection);
        
        // Test 1: Send Property Request
        
        // Send a property request
        property_client->send_get_property_data("ResourceList", "", "", -1, -1);
        
        // Check the sent data to extract request ID from raw message
        ASSERT_FALSE(client_to_server_data.empty()) << "Should have sent data to server";
        ASSERT_GE(client_to_server_data.size(), 14) << "Message should be long enough";
        uint8_t actual_request_id = client_to_server_data[13];
        EXPECT_GT(actual_request_id, 0) << "Request ID should be non-zero";
        
        // Test 2: Simulate Server Processing
        
        // Simulate receiving the request on the server
        ASSERT_FALSE(client_to_server_data.empty());
        EXPECT_NO_THROW(server_device->get_messenger().process_input(0, client_to_server_data));
        
        // Test 3: Send Reply Back to Client
        
        // The server should have sent a reply - process on client
        if (!server_to_client_data.empty()) {
            // Extract request ID from reply
            if (server_to_client_data.size() >= 14) {
                uint8_t reply_request_id = server_to_client_data[13];
                EXPECT_EQ(actual_request_id, reply_request_id) << "Reply should have same request ID as original request";
            }
            
            EXPECT_NO_THROW(client_device->get_messenger().process_input(0, server_to_client_data));
        }
        
        // Test 4: Multiple Concurrent Requests
        
        // Send multiple requests quickly
        property_client->send_get_property_data("DeviceInfo", "", "", -1, -1);
        std::vector<uint8_t> request2_data = client_to_server_data;
        
        property_client->send_get_property_data("ChannelList", "", "", -1, -1);  
        std::vector<uint8_t> request3_data = client_to_server_data;
        
        // Process each request on server and get replies
        EXPECT_NO_THROW(server_device->get_messenger().process_input(0, request2_data));
        std::vector<uint8_t> reply2_data = server_to_client_data;
        
        EXPECT_NO_THROW(server_device->get_messenger().process_input(0, request3_data));
        std::vector<uint8_t> reply3_data = server_to_client_data;
        
        // Process replies on client
        if (!reply2_data.empty()) {
            EXPECT_NO_THROW(client_device->get_messenger().process_input(0, reply2_data));
        }
        
        if (!reply3_data.empty()) {
            EXPECT_NO_THROW(client_device->get_messenger().process_input(0, reply3_data));
        }
        
        // Test Results - Check for request ID issues in log
        bool found_mismatch = false;
        for (const auto& msg : log_messages) {
            if (msg.find("NO MATCHING request") != std::string::npos) {
                found_mismatch = true;
                break;
            }
        }
        
        EXPECT_FALSE(found_mismatch) << "Request ID correlation issues detected in debug logs";
}