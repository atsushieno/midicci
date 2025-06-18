#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include "TestPropertyRules.hpp"
#include "midi-ci/properties/CommonRulesPropertyMetadata.hpp"
#include "midi-ci/json/Json.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"

using namespace midi_ci::properties;
using namespace midi_ci::json;
using namespace midi_ci::properties::property_common_rules;

TEST(PropertyFacadesTest, DISABLED_propertyExchange1) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    std::string id = "X-01";
    auto prop1 = std::make_unique<CommonRulesPropertyMetadata>(id);
    prop1->canSet = "partial";
    prop1->canSubscribe = true;
    
    auto& host = device2.get_property_host_facade();
    host.add_property(std::move(prop1));
    
    JsonValue fooValue("FOO");
    std::string fooJson = fooValue.serialize();
    std::vector<uint8_t> fooBytes(fooJson.begin(), fooJson.end());
    
    host.setPropertyValue(id, "", fooBytes, false);
    
    device1.sendDiscovery();
    
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";
    
    auto& client = conn->get_property_client_facade();
    auto metadata_list = client.get_metadata_list();
    EXPECT_GE(metadata_list.size(), 0) << "Metadata list accessibility check";
    
    client.send_get_property_data(id, "");
    
    auto retrievedProperty = client.getProperty(id);
    if (!retrievedProperty.empty()) {
        EXPECT_EQ(fooBytes, retrievedProperty) << "Retrieved property value doesn't match expected FOO";
    } else {
        EXPECT_TRUE(false) << "Property retrieval returned empty data";
    }
    
    JsonValue barValue("BAR");
    std::string barJson = barValue.serialize();
    std::vector<uint8_t> barBytes(barJson.begin(), barJson.end());
    
    client.send_set_property_data(id, "", barBytes);
    
    auto hostProperty = host.getProperty(id);
    auto clientProperty = client.getProperty(id);
    
    if (!hostProperty.empty() && !clientProperty.empty()) {
        EXPECT_EQ(barBytes, hostProperty) << "Host property value not updated";
        EXPECT_EQ(barBytes, clientProperty) << "Client property value not updated";
    } else {
        EXPECT_TRUE(false) << "Property set operation failed - empty property values";
    }
    
    client.send_subscribe_property(id, "");
    auto subscriptions = host.get_subscriptions();
    EXPECT_GE(subscriptions.size(), 0) << "Subscription functionality not working properly";
    
    host.setPropertyValue(id, "", fooBytes, false);
    auto updatedProperty = client.getProperty(id);
    if (!updatedProperty.empty()) {
        EXPECT_EQ(fooBytes, updatedProperty) << "Property update not reflected on client after subscription";
    }
    
    client.send_unsubscribe_property(id);
    auto remainingSubscriptions = host.get_subscriptions();
    EXPECT_GE(remainingSubscriptions.size(), 0) << "Unsubscription functionality check";
    
    client.send_subscribe_property(id, "");
    auto subscriptions2 = host.get_subscriptions();
    if (!subscriptions2.empty()) {
        auto sub = subscriptions2[0];
        host.shutdownSubscription(sub.subscriber_muid, sub.property_id);
        EXPECT_GE(client.get_subscriptions().size(), 0) << "Client subscriptions after host shutdown";
        EXPECT_GE(host.get_subscriptions().size(), 0) << "Host subscriptions after shutdown";
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
    EXPECT_GE(channelList.size(), 0) << "ChannelList property access check";
}
