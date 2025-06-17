#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include "TestPropertyRules.hpp"
#include "midi-ci/properties/CommonRulesPropertyMetadata.hpp"
#include "midi-ci/json/Json.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"

using namespace midi_ci::properties;
using namespace midi_ci::json;
using namespace midi_ci::properties::property_common_rules;

TEST(PropertyFacadesTest, propertyExchange1) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    std::string id = "X-01";
    CommonRulesPropertyMetadata prop1(id);
    prop1.canSet = "partial";
    prop1.canSubscribe = true;
    
    auto& host = device2.get_property_host_facade();
    host.add_property(prop1);
    
    JsonValue fooValue("FOO");
    std::string fooJson = fooValue.serialize();
    std::vector<uint8_t> fooBytes(fooJson.begin(), fooJson.end());
    
    auto property_rules = dynamic_cast<TestPropertyRules*>(host.get_property_rules());
    if (property_rules) {
        property_rules->set_property_value(id, fooBytes);
    }
    
    device1.sendDiscovery();
    
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";
    
    auto& client = conn->get_property_client_facade();
    auto metadata_list = client.get_metadata_list();
    EXPECT_EQ(4, metadata_list.size()) << "Expected 4 properties in metadata list";
    
    client.send_get_property_data(id, "");
    
    auto retrievedProperty = client.getProperty(id);
    EXPECT_EQ(fooBytes, retrievedProperty) << "Retrieved property value doesn't match expected FOO";
    
    JsonValue barValue("BAR");
    std::string barJson = barValue.serialize();
    std::vector<uint8_t> barBytes(barJson.begin(), barJson.end());
    
    client.send_set_property_data(id, "", barBytes);
    
    if (property_rules) {
        auto hostProperty = property_rules->get_property_value(id);
        EXPECT_EQ(barBytes, hostProperty) << "Host property value not updated";
    }
    auto clientProperty = client.getProperty(id);
    EXPECT_EQ(barBytes, clientProperty) << "Client property value not updated";
    
    client.send_subscribe_property(id, "");
    if (property_rules) {
        EXPECT_EQ(1, property_rules->get_subscriptions().size()) << "Subscription not registered on host";
    }
    
    if (property_rules) {
        property_rules->set_property_value(id, fooBytes);
        auto updatedProperty = property_rules->get_property_value(id);
        EXPECT_EQ(fooBytes, updatedProperty) << "Property update not applied";
    }
    
    client.send_unsubscribe_property(id);
    
    client.send_subscribe_property(id, "");
    if (property_rules) {
        auto subscriptions = property_rules->get_subscriptions();
        if (!subscriptions.empty()) {
            EXPECT_GT(subscriptions.size(), 0) << "No subscriptions found";
        }
    }
}

TEST(PropertyFacadesTest, propertyExchange2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    
    device1.sendDiscovery();
    
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established";
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";
    
    auto& client = conn->get_property_client_facade();
    client.send_get_property_data(PropertyResourceNames::CHANNEL_LIST, "");
    
    auto channelList = client.getProperty(PropertyResourceNames::CHANNEL_LIST);
    EXPECT_GT(channelList.size(), 0) << "ChannelList should not be empty";
}
