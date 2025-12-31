#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include <thread>
#include <chrono>
#include "midicci/midicci.hpp"

using namespace midicci;
using namespace midicci::commonproperties;

class FinalCorrelationTest : public ::testing::Test {
protected:
    // Transport simulation that routes messages between devices
    std::vector<uint8_t> client_to_server_queue;
    std::vector<uint8_t> server_to_client_queue;

    std::function<bool(uint8_t, const std::vector<uint8_t>&)> client_transport = [this](uint8_t group, const std::vector<uint8_t>& data) {
        // CLIENT->SERVER: Group and Size logged
        this->client_to_server_queue = data;
        return true;
    };

    std::function<bool(uint8_t, const std::vector<uint8_t>&)> server_transport = [this](uint8_t group, const std::vector<uint8_t>& data) {
        // SERVER->CLIENT: Group and Size logged
        this->server_to_client_queue = data;
        return true;
    };

    // Debug logger
    std::function<void(const std::string&, bool)> debug_logger = [](const std::string& message, bool is_outgoing) {
        std::string direction = is_outgoing ? "OUT" : "IN ";
        // Direction and message logged
    };
};

TEST_F(FinalCorrelationTest, BasicRequestIdCorrelation) {
    // Create devices
    MidiCIDeviceConfiguration client_config;
    MidiCIDeviceConfiguration server_config;
    
    auto client_device = std::make_shared<MidiCIDevice>(0x12345678, client_config);
    auto server_device = std::make_shared<MidiCIDevice>(0x87654321, server_config);
    
    // Set up logging and transport
    client_device->set_logger(debug_logger);
    server_device->set_logger(debug_logger);
    client_device->set_sysex_sender(client_transport);
    server_device->set_sysex_sender(server_transport);
    
    // Add test property to server
    auto test_property = std::make_unique<CommonRulesPropertyMetadata>("TestProperty");
    test_property->canGet = true;
    test_property->canSet = false;
    server_device->get_property_host_facade().addMetadata(std::move(test_property));
    
    // Create client connection
    auto connection = std::make_shared<ClientConnection>(*client_device, 0x87654321, 
        DeviceDetails{0x123, 0x456, 0x789, 0xABC}, 4096);
    auto property_client = std::make_unique<PropertyClientFacade>(*client_device, *connection);
    
    // Send property request
    property_client->send_get_property_data("ResourceList", "", "", -1, -1);
    
    EXPECT_FALSE(client_to_server_queue.empty());
    
    // Extract request ID from sent message
    uint8_t sent_request_id = client_to_server_queue[13];
    // Sent request with ID logged
    
    // Check message format - message size should be reasonable
    EXPECT_GT(client_to_server_queue.size(), 20u);
    
    // Process request on server
    server_device->get_messenger().process_input(0, client_to_server_queue);
    
    EXPECT_FALSE(server_to_client_queue.empty());
    
    // Extract request ID from reply
    uint8_t reply_request_id = server_to_client_queue[13];
    // Server replied with request ID logged
    
    EXPECT_EQ(sent_request_id, reply_request_id);
    
    // Process reply on client
    client_device->get_messenger().process_input(0, server_to_client_queue);
}

TEST_F(FinalCorrelationTest, MultipleConcurrentRequests) {
    // Create devices
    MidiCIDeviceConfiguration client_config;
    MidiCIDeviceConfiguration server_config;
    
    auto client_device = std::make_shared<MidiCIDevice>(0x12345678, client_config);
    auto server_device = std::make_shared<MidiCIDevice>(0x87654321, server_config);
    
    // Set up logging and transport
    client_device->set_logger(debug_logger);
    server_device->set_logger(debug_logger);
    client_device->set_sysex_sender(client_transport);
    server_device->set_sysex_sender(server_transport);
    
    // Add test property to server
    auto test_property = std::make_unique<CommonRulesPropertyMetadata>("TestProperty");
    test_property->canGet = true;
    test_property->canSet = false;
    server_device->get_property_host_facade().addMetadata(std::move(test_property));
    
    // Create client connection
    auto connection = std::make_shared<ClientConnection>(*client_device, 0x87654321, 
        DeviceDetails{0x123, 0x456, 0x789, 0xABC}, 4096);
    auto property_client = std::make_unique<PropertyClientFacade>(*client_device, *connection);
    
    // Send multiple requests to test correlation
    property_client->send_get_property_data("DeviceInfo", "", "", -1, -1);
    auto request2_data = client_to_server_queue;
    uint8_t request2_id = request2_data[13];
    
    property_client->send_get_property_data("ChannelList", "", "", -1, -1);
    auto request3_data = client_to_server_queue;
    uint8_t request3_id = request3_data[13];
    
    // Sent requests with different IDs
    EXPECT_NE(request2_id, request3_id);
    
    // Process request 3 first (out of order)
    server_device->get_messenger().process_input(0, request3_data);
    auto reply3_data = server_to_client_queue;
    uint8_t reply3_id = reply3_data[13];
    
    // Process request 2
    server_device->get_messenger().process_input(0, request2_data);
    auto reply2_data = server_to_client_queue;
    uint8_t reply2_id = reply2_data[13];
    
    // Server replied with matching IDs
    
    // Verify request IDs are preserved
    EXPECT_EQ(request3_id, reply3_id);
    EXPECT_EQ(request2_id, reply2_id);
    
    // Process replies on client (out of order)
    client_device->get_messenger().process_input(0, reply3_data);
    client_device->get_messenger().process_input(0, reply2_data);
    
    // All tests passed - Request ID correlation is working correctly
}
