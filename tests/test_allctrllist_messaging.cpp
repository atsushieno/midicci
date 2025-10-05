#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include "AllCtrlListTestData.hpp"
#include <midicci/midicci.hpp>
#include <cstring>

using namespace midicci;
using namespace midicci::commonproperties;
using namespace midicci::commonproperties::StandardPropertiesExtensions;

class AllCtrlListMessagingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configuration is set up in TestCIMediator
    }
};

TEST_F(AllCtrlListMessagingTest, ClientServerPropertyExchange) {
    // Create mediator to handle communication between two devices
    TestCIMediator mediator;
    auto& server = mediator.getDevice2();  // device2 is the property host (server)
    auto& client = mediator.getDevice1();  // device1 is the property client

    // Parse the AllCtrlList test data
    const char* json_str = test::ALL_CTRL_LIST_OPNPLUG_AE;
    std::vector<uint8_t> json_data(json_str, json_str + std::strlen(json_str));

    // Parse the control list to verify it's valid
    auto controls = StandardProperties::parseControlList(json_data);
    ASSERT_FALSE(controls.empty()) << "Failed to parse AllCtrlList test data";

    // Add the AllCtrlList metadata to the server's property host
    auto& host = server.get_property_host_facade();
    auto metadata = std::make_unique<CommonRulesPropertyMetadata>(StandardProperties::allCtrlListMetadata);
    host.addMetadata(std::move(metadata));

    // Set the AllCtrlList on the server device
    setAllCtrlList(server, controls);

    // Verify the AllCtrlList was set on the server
    auto server_controls = getAllCtrlList(server);
    ASSERT_TRUE(server_controls.has_value()) << "AllCtrlList not set on server";
    EXPECT_EQ(controls.size(), server_controls->size()) << "AllCtrlList size mismatch on server";

    // Perform discovery to establish connection
    client.sendDiscovery();

    // Verify connection was established
    auto connections = client.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";

    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";

    // Get the property client facade
    auto& property_client = conn->get_property_client_facade();

    // Send GetPropertyData request for AllCtrlList from client to server
    property_client.send_get_property_data(StandardPropertyNames::ALL_CTRL_LIST, "", "");

    // The server should have received the request and sent a reply
    // The client should have processed the reply and stored it

    // Retrieve the AllCtrlList from the client's property cache
    auto client_values = property_client.get_properties()->getValues();

    // Find the AllCtrlList property in the client's cache
    auto it = std::find_if(client_values.begin(), client_values.end(),
        [](const PropertyValue& pv) {
            return pv.id == StandardPropertyNames::ALL_CTRL_LIST;
        });

    ASSERT_NE(it, client_values.end()) << "AllCtrlList not found in client properties after GetPropertyData";

    // Parse the received AllCtrlList data
    auto received_controls = StandardProperties::parseControlList(it->body);
    ASSERT_FALSE(received_controls.empty()) << "Failed to parse received AllCtrlList data";

    // Verify the received data matches what we set on the server
    EXPECT_EQ(controls.size(), received_controls.size()) << "Received AllCtrlList size doesn't match original";

    // Verify a few known controls from the test data
    bool found_master_volume = false;
    bool found_emulator = false;

    for (const auto& ctrl : received_controls) {
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

    EXPECT_TRUE(found_master_volume) << "Master volume control not found in received data";
    EXPECT_TRUE(found_emulator) << "Emulator control not found in received data";
}

TEST_F(AllCtrlListMessagingTest, MultiplePropertyExchanges) {
    // Create mediator to handle communication between two devices
    TestCIMediator mediator;
    auto& server = mediator.getDevice2();
    auto& client = mediator.getDevice1();

    // Parse the AllCtrlList test data
    const char* json_str = test::ALL_CTRL_LIST_OPNPLUG_AE;
    std::vector<uint8_t> json_data(json_str, json_str + std::strlen(json_str));
    auto controls = StandardProperties::parseControlList(json_data);
    ASSERT_FALSE(controls.empty());

    // Add the AllCtrlList metadata to the server's property host
    auto& host = server.get_property_host_facade();
    auto metadata = std::make_unique<CommonRulesPropertyMetadata>(StandardProperties::allCtrlListMetadata);
    host.addMetadata(std::move(metadata));

    // Set the AllCtrlList on the server device
    setAllCtrlList(server, controls);

    // Perform discovery
    client.sendDiscovery();

    auto connections = client.get_connections();
    ASSERT_GT(connections.size(), 0);
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    auto& property_client = conn->get_property_client_facade();

    // First request for AllCtrlList
    property_client.send_get_property_data(StandardPropertyNames::ALL_CTRL_LIST, "", "");

    auto client_values = property_client.get_properties()->getValues();

    auto it = std::find_if(client_values.begin(), client_values.end(),
        [](const PropertyValue& pv) {
            return pv.id == StandardPropertyNames::ALL_CTRL_LIST;
        });

    ASSERT_NE(it, client_values.end()) << "AllCtrlList not found after first request";
    auto first_body = it->body;

    // Second request for the same property
    property_client.send_get_property_data(StandardPropertyNames::ALL_CTRL_LIST, "", "");

    client_values = property_client.get_properties()->getValues();
    it = std::find_if(client_values.begin(), client_values.end(),
        [](const PropertyValue& pv) {
            return pv.id == StandardPropertyNames::ALL_CTRL_LIST;
        });

    ASSERT_NE(it, client_values.end()) << "AllCtrlList not found after second request";

    // Verify both requests return the same data
    EXPECT_EQ(first_body, it->body) << "AllCtrlList data changed between requests";
}

TEST_F(AllCtrlListMessagingTest, EmptyAllCtrlList) {
    // Test handling of empty AllCtrlList
    TestCIMediator mediator;
    auto& server = mediator.getDevice2();
    auto& client = mediator.getDevice1();

    // Add the AllCtrlList metadata to the server's property host
    auto& host = server.get_property_host_facade();
    auto metadata = std::make_unique<CommonRulesPropertyMetadata>(StandardProperties::allCtrlListMetadata);
    host.addMetadata(std::move(metadata));

    // Set an empty AllCtrlList on the server
    std::vector<MidiCIControl> empty_controls;
    setAllCtrlList(server, empty_controls);

    // Perform discovery
    client.sendDiscovery();

    auto connections = client.get_connections();
    ASSERT_GT(connections.size(), 0);
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    auto& property_client = conn->get_property_client_facade();

    // Request the empty AllCtrlList
    property_client.send_get_property_data(StandardPropertyNames::ALL_CTRL_LIST, "", "");

    auto client_values = property_client.get_properties()->getValues();

    auto it = std::find_if(client_values.begin(), client_values.end(),
        [](const PropertyValue& pv) {
            return pv.id == StandardPropertyNames::ALL_CTRL_LIST;
        });

    ASSERT_NE(it, client_values.end()) << "AllCtrlList not found in client properties";

    // Parse the received data
    auto received_controls = StandardProperties::parseControlList(it->body);

    // Empty list should parse successfully and be empty
    EXPECT_TRUE(received_controls.empty()) << "Expected empty AllCtrlList";
}
