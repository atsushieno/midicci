#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include "TestPropertyRules.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/Json.hpp"
#include "midicci/PropertyCommonRules.hpp"

using namespace midicci::commonproperties;
using namespace midicci;

TEST(PropertyFacadesTest, propertyExchange1) {
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

    client.send_get_property_data(id, "");

    JsonValue barValue("BAR");
    std::string barJson = barValue.serialize();
    std::vector<uint8_t> barBytes(barJson.begin(), barJson.end());
    
    client.send_set_property_data(id, "", barBytes);
    
    EXPECT_EQ(barBytes, host.getProperty(id)) << "Host property value not updated";

    client.send_subscribe_property(id, "");
    EXPECT_EQ(1, host.get_subscriptions().size()) << "Subscription not registered on host";

    client.send_unsubscribe_property(id);
    EXPECT_EQ(0, host.get_subscriptions().size()) << "Subscription not removed after unsubscription";
    
    client.send_subscribe_property(id, "");
    EXPECT_EQ(1, host.get_subscriptions().size()) << "Subscription not registered on host, 2nd time";
    auto subscriptions = host.get_subscriptions();
    if (!subscriptions.empty()) {
        auto sub = subscriptions[0];
        host.shutdownSubscription(sub.subscriber_muid, sub.property_id);
        EXPECT_EQ(0, client.get_subscriptions().size()) << "Client subscriptions not cleared after host shutdown";
        EXPECT_EQ(0, host.get_subscriptions().size()) << "Host subscriptions not cleared after shutdown";
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
}
