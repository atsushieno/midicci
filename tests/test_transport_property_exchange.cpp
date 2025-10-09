#include <gtest/gtest.h>
#include "TestCITransport.hpp"
#include <midicci/midicci.hpp>

using namespace midicci;
using namespace midicci::test;
using namespace midicci::commonproperties;

/**
 * Test suite for property exchange over virtual UMP ports.
 * These tests verify that GetPropertyData and GetPropertyDataReply messages
 * work correctly when sent through actual MIDI transport (virtual ports).
 */

TEST(TransportPropertyExchangeTest, BasicGetPropertyDataOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Setup: Device2 hosts a property
    std::string property_id = "X-TestProperty-01";
    auto prop_metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    prop_metadata->canSet = "partial";
    prop_metadata->canSubscribe = false;

    auto& host = device2.get_property_host_facade();
    host.addMetadata(std::move(prop_metadata));

    // Set initial property value on host (Device2)
    JsonValue initial_value("InitialValue");
    std::string initial_json = initial_value.serialize();
    std::vector<uint8_t> initial_bytes(initial_json.begin(), initial_json.end());
    host.setPropertyValue(property_id, "", initial_bytes, false);

    // Step 1: Device1 sends discovery to establish connection over transport
    device1.sendDiscovery();

    // Wait for discovery to complete over the virtual ports
    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));

    ASSERT_TRUE(discovery_complete) << "Discovery did not complete over transport";
    ASSERT_GT(device1.get_connections().size(), 0) << "No connections after discovery";

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";

    // Step 2: Client (Device1) requests property data over transport
    auto& client = conn->get_property_client_facade();
    client.send_get_property_data(property_id, "", "");

    // Wait for the property data to be received over transport
    transport.processMessages(std::chrono::milliseconds(500));

    // Step 3: Verify the property value was received
    bool property_received = transport.waitForCondition([&client, &property_id, &initial_bytes]() {
        auto* properties = client.get_properties();
        auto values = properties->getValues();
        auto it = std::find_if(values.begin(), values.end(),
                              [&property_id](const PropertyValue& pv) {
                                  return pv.id == property_id;
                              });
        // Wait until we have the property AND it has body data
        return it != values.end() && it->body == initial_bytes;
    }, std::chrono::milliseconds(2000));

    ASSERT_TRUE(property_received) << "Property data was not received over transport";

    auto* properties = client.get_properties();
    auto values = properties->getValues();
    auto it = std::find_if(values.begin(), values.end(),
                          [&property_id](const PropertyValue& pv) {
                              return pv.id == property_id;
                          });

    ASSERT_NE(it, values.end()) << "Property not found in client properties";
    EXPECT_EQ(initial_bytes, it->body) << "Property value mismatch";
}

TEST(TransportPropertyExchangeTest, SetPropertyDataOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Setup: Device2 hosts a property that can be set
    std::string property_id = "X-WritableProperty-01";
    auto prop_metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    prop_metadata->canSet = "full";
    prop_metadata->canSubscribe = false;

    auto& host = device2.get_property_host_facade();
    host.addMetadata(std::move(prop_metadata));

    // Set initial value
    JsonValue initial_value("OldValue");
    std::string initial_json = initial_value.serialize();
    std::vector<uint8_t> initial_bytes(initial_json.begin(), initial_json.end());
    host.setPropertyValue(property_id, "", initial_bytes, false);

    // Establish connection
    device1.sendDiscovery();
    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));
    ASSERT_TRUE(discovery_complete);

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    // Client sets new property value
    JsonValue new_value("NewValue");
    std::string new_json = new_value.serialize();
    std::vector<uint8_t> new_bytes(new_json.begin(), new_json.end());

    auto& client = conn->get_property_client_facade();
    client.send_set_property_data(property_id, "", new_bytes);

    // Wait for the set operation to complete
    transport.processMessages(std::chrono::milliseconds(500));

    // Verify the host's property value was updated
    bool value_updated = transport.waitForCondition([&host, &property_id, &new_bytes]() {
        auto values = host.get_properties().getValues();
        auto it = std::find_if(values.begin(), values.end(),
                              [&property_id](const PropertyValue& pv) {
                                  return pv.id == property_id;
                              });
        return it != values.end() && it->body == new_bytes;
    }, std::chrono::milliseconds(2000));

    ASSERT_TRUE(value_updated) << "Host property value was not updated";

    auto values = host.get_properties().getValues();
    auto it = std::find_if(values.begin(), values.end(),
                          [&property_id](const PropertyValue& pv) {
                              return pv.id == property_id;
                          });
    ASSERT_NE(it, values.end());
    EXPECT_EQ(new_bytes, it->body) << "Property value not updated correctly";
}

TEST(TransportPropertyExchangeTest, LargePropertyDataOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Setup: Device2 hosts a property with large data (tests fragmentation)
    std::string property_id = "X-LargeProperty-01";
    auto prop_metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    prop_metadata->canSet = "none";
    prop_metadata->canSubscribe = false;

    auto& host = device2.get_property_host_facade();
    host.addMetadata(std::move(prop_metadata));

    // Create a large JSON array to test fragmentation handling
    std::string large_json = "[";
    for (int i = 0; i < 1000; i++) {
        if (i > 0) large_json += ",";
        large_json += "{\"index\":" + std::to_string(i) + ",\"data\":\"test_data_" + std::to_string(i) + "\"}";
    }
    large_json += "]";

    std::vector<uint8_t> large_bytes(large_json.begin(), large_json.end());
    host.setPropertyValue(property_id, "", large_bytes, false);

    std::cout << "Large property size: " << large_bytes.size() << " bytes" << std::endl;

    // Establish connection
    device1.sendDiscovery();
    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));
    ASSERT_TRUE(discovery_complete);

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    // Request the large property
    auto& client = conn->get_property_client_facade();
    client.send_get_property_data(property_id, "", "");

    // Wait longer for large data transfer
    transport.processMessages(std::chrono::milliseconds(1000));

    // Verify the large property was received correctly
    bool property_received = transport.waitForCondition([&client, &property_id, &large_bytes]() {
        auto* properties = client.get_properties();
        auto values = properties->getValues();
        auto it = std::find_if(values.begin(), values.end(),
                              [&property_id](const PropertyValue& pv) {
                                  return pv.id == property_id;
                              });
        // Wait until we have the property AND it has the complete body data
        return it != values.end() && it->body.size() == large_bytes.size() && it->body == large_bytes;
    }, std::chrono::milliseconds(5000));

    ASSERT_TRUE(property_received) << "Large property data was not received";

    auto* properties = client.get_properties();
    auto values = properties->getValues();
    auto it = std::find_if(values.begin(), values.end(),
                          [&property_id](const PropertyValue& pv) {
                              return pv.id == property_id;
                          });

    ASSERT_NE(it, values.end());
    EXPECT_EQ(large_bytes.size(), it->body.size()) << "Large property size mismatch";
    EXPECT_EQ(large_bytes, it->body) << "Large property data corrupted";
}

TEST(TransportPropertyExchangeTest, MultiplePropertyRequestsOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Setup: Device2 hosts multiple properties
    std::vector<std::string> property_ids = {
        "X-Property-A",
        "X-Property-B",
        "X-Property-C"
    };

    auto& host = device2.get_property_host_facade();

    for (const auto& prop_id : property_ids) {
        auto prop_metadata = std::make_unique<CommonRulesPropertyMetadata>(prop_id);
        prop_metadata->canSet = "none";
        prop_metadata->canSubscribe = false;
        host.addMetadata(std::move(prop_metadata));

        JsonValue value("Value_for_" + prop_id);
        std::string json = value.serialize();
        std::vector<uint8_t> bytes(json.begin(), json.end());
        host.setPropertyValue(prop_id, "", bytes, false);
    }

    // Establish connection
    device1.sendDiscovery();
    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));
    ASSERT_TRUE(discovery_complete);

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    // Request all properties
    auto& client = conn->get_property_client_facade();
    for (const auto& prop_id : property_ids) {
        client.send_get_property_data(prop_id, "", "");
        transport.processMessages(std::chrono::milliseconds(100));
    }

    // Verify all properties were received
    bool all_received = transport.waitForCondition([&client, &property_ids]() {
        auto* properties = client.get_properties();
        auto values = properties->getValues();

        for (const auto& prop_id : property_ids) {
            bool found = std::any_of(values.begin(), values.end(),
                                    [&prop_id](const PropertyValue& pv) {
                                        return pv.id == prop_id;
                                    });
            if (!found) return false;
        }
        return true;
    }, std::chrono::milliseconds(3000));

    ASSERT_TRUE(all_received) << "Not all properties were received";

    auto* properties = client.get_properties();
    auto values = properties->getValues();

    for (const auto& prop_id : property_ids) {
        auto it = std::find_if(values.begin(), values.end(),
                              [&prop_id](const PropertyValue& pv) {
                                  return pv.id == prop_id;
                              });
        ASSERT_NE(it, values.end()) << "Property " << prop_id << " not received";
    }
}

TEST(TransportPropertyExchangeTest, PropertySubscriptionOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Setup: Device2 hosts a subscribable property
    std::string property_id = "X-SubscribableProperty-01";
    auto prop_metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
    prop_metadata->canSet = "partial";
    prop_metadata->canSubscribe = true;

    auto& host = device2.get_property_host_facade();
    host.addMetadata(std::move(prop_metadata));

    JsonValue initial_value("Initial");
    std::string initial_json = initial_value.serialize();
    std::vector<uint8_t> initial_bytes(initial_json.begin(), initial_json.end());
    host.setPropertyValue(property_id, "", initial_bytes, false);

    // Establish connection
    device1.sendDiscovery();
    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));
    ASSERT_TRUE(discovery_complete);

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    // Subscribe to the property
    auto& client = conn->get_property_client_facade();
    client.send_subscribe_property(property_id, "", "");

    transport.processMessages(std::chrono::milliseconds(500));

    // Verify subscription was registered
    bool subscribed = transport.waitForCondition([&host]() {
        return host.get_subscriptions().size() > 0;
    }, std::chrono::milliseconds(2000));

    ASSERT_TRUE(subscribed) << "Subscription was not registered on host";
    EXPECT_EQ(1, host.get_subscriptions().size());

    // Unsubscribe
    client.send_unsubscribe_property(property_id, "");
    transport.processMessages(std::chrono::milliseconds(500));

    // Verify subscription was removed
    bool unsubscribed = transport.waitForCondition([&host]() {
        return host.get_subscriptions().size() == 0;
    }, std::chrono::milliseconds(2000));

    ASSERT_TRUE(unsubscribed) << "Subscription was not removed";
    EXPECT_EQ(0, host.get_subscriptions().size());
}

TEST(TransportPropertyExchangeTest, StandardPropertyChannelListOverTransport) {
    TestCITransport transport;
    if (!transport.isRunnable())
        return;
    auto& device1 = transport.getDevice1();
    auto& device2 = transport.getDevice2();

    // Device2 should have standard properties available
    device1.sendDiscovery();

    bool discovery_complete = transport.waitForCondition([&device1]() {
        return device1.get_connections().size() > 0;
    }, std::chrono::milliseconds(2000));
    ASSERT_TRUE(discovery_complete);

    auto connections = device1.get_connections();
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    // Request standard ChannelList property
    auto& client = conn->get_property_client_facade();
    client.send_get_property_data(PropertyResourceNames::CHANNEL_LIST, "", "");

    transport.processMessages(std::chrono::milliseconds(1000));

    // Note: The response depends on Device2's configuration
    // This test verifies that the request doesn't crash and gets processed
    transport.pumpMessages();

    // Success if we get here without crashes
    SUCCEED();
}
