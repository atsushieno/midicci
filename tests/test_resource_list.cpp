#include <memory>
#include "midicci/midicci.hpp"
#include <gtest/gtest.h>

using namespace midicci;
using namespace midicci::commonproperties;

class ResourceListTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a device configuration
        config = std::make_unique<MidiCIDeviceConfiguration>();
        
        // Create device with a simple logging callback
        device = std::make_shared<MidiCIDevice>(0x12345678, *config, 
            [](const LogData& log_data) {
                // Log messages are handled by gtest framework
            });
        
        // Create property host facade
        facade = std::make_unique<PropertyHostFacade>(*device, *config);
    }

    std::unique_ptr<MidiCIDeviceConfiguration> config;
    std::shared_ptr<MidiCIDevice> device;
    std::unique_ptr<PropertyHostFacade> facade;
};

TEST_F(ResourceListTest, UserDefinedPropertyInResourceList) {
    // Add a user-defined property using the new API
    std::string property_id = "X-TEST-PROPERTY";
    auto test_property = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    test_property->canGet = true;
    test_property->canSet = "full";
    test_property->canSubscribe = true;
    test_property->mediaTypes = {"application/json"};
    test_property->encodings = {"UTF-8"};
    test_property->schema = R"({"type": "string"})";
    
    // Set some test data
    std::string test_data = R"({"message": "Hello from user property!"})";
    std::vector<uint8_t> data_bytes(test_data.begin(), test_data.end());
    test_property->setData(data_bytes);
    
    // Add the property to the facade
    facade->addMetadata(std::move(test_property));
    
    // Test ResourceList request
    Common common(0x12345678, 0x87654321, ADDRESS_FUNCTION_BLOCK, 0);
    
    // Create header for ResourceList request
    std::string header_str = R"({"resource":"ResourceList"})";
    std::vector<uint8_t> header_bytes(header_str.begin(), header_str.end());
    
    GetPropertyData request(common, 1, header_bytes);
    
    // Process the request
    auto reply = facade->process_get_property_data(request);
    
    // Check the response
    std::string reply_header_str(reply.get_header().begin(), reply.get_header().end());
    std::string reply_body_str(reply.get_body().begin(), reply.get_body().end());
    
    // Verify that our property is in the ResourceList
    EXPECT_NE(reply_body_str.find(property_id), std::string::npos)
        << "User property '" << property_id << "' should be found in ResourceList. "
        << "Response body: " << reply_body_str;
}

TEST_F(ResourceListTest, GetUserDefinedPropertyData) {
    // Add a user-defined property
    std::string property_id = "X-TEST-PROPERTY";
    auto test_property = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    test_property->canGet = true;
    test_property->canSet = "full";
    test_property->canSubscribe = true;
    test_property->mediaTypes = {"application/json"};
    test_property->encodings = {"UTF-8"};
    test_property->schema = R"({"type": "string"})";
    
    // Set some test data
    std::string test_data = R"({"message": "Hello from user property!"})";
    std::vector<uint8_t> data_bytes(test_data.begin(), test_data.end());
    test_property->setData(data_bytes);
    
    facade->addMetadata(std::move(test_property));
    
    // Test GetPropertyData for the user property
    Common common(0x12345678, 0x87654321, ADDRESS_FUNCTION_BLOCK, 0);
    
    std::string user_header_str = R"({"resource":")" + property_id + R"("})";
    std::vector<uint8_t> user_header_bytes(user_header_str.begin(), user_header_str.end());
    
    GetPropertyData user_request(common, 2, user_header_bytes);
    auto user_reply = facade->process_get_property_data(user_request);
    
    std::string user_reply_header_str(user_reply.get_header().begin(), user_reply.get_header().end());
    std::string user_reply_body_str(user_reply.get_body().begin(), user_reply.get_body().end());
    
    // Verify that we got the expected data
    EXPECT_NE(user_reply_body_str.find("Hello from user property!"), std::string::npos)
        << "User property data should contain expected message. "
        << "Response body: " << user_reply_body_str;
}