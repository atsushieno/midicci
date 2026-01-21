#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include <midicci/midicci.hpp>

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
    
    auto& host = device2.getPropertyHostFacade();
    host.addMetadata(std::move(prop1));
    
    JsonValue fooValue("FOO");
    std::string fooJson = fooValue.serialize();
    std::vector<uint8_t> fooBytes(fooJson.begin(), fooJson.end());
    
    host.setPropertyValue(id, "", fooBytes, false);
    
    device1.sendDiscovery();
    
    auto connections = device1.getConnections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";
    
    auto& client = conn->getPropertyClientFacade();

    client.sendGetPropertyData(id, "", "");

    JsonValue barValue("BAR");
    std::string barJson = barValue.serialize();
    std::vector<uint8_t> barBytes(barJson.begin(), barJson.end());
    
    client.sendSetPropertyData(id, "", barBytes);
    
    auto values = host.getProperties().getValues();
    auto it = std::find_if(values.begin(), values.end(),
        [&id](const PropertyValue& pv) { return pv.id == id; });
    ASSERT_NE(it, values.end()) << "Property " << id << " not found";
    EXPECT_EQ(barBytes, it->body) << "Host property value not updated";

    client.sendSubscribeProperty(id, "", "");
    EXPECT_EQ(1, host.getSubscriptions().size()) << "Subscription not registered on host";

    client.sendUnsubscribeProperty(id, "");
    EXPECT_EQ(0, host.getSubscriptions().size()) << "Subscription not removed after unsubscription";
    
    client.sendSubscribeProperty(id, "", "");
    EXPECT_EQ(1, host.getSubscriptions().size()) << "Subscription not registered on host, 2nd time";
    auto subscriptions = host.getSubscriptions();
    if (!subscriptions.empty()) {
        auto sub = subscriptions[0];
        std::cout << "Before host shutdown - host subscriptions: " << host.getSubscriptions().size() << ", client subscriptions: " << client.getSubscriptions().size() << std::endl;
        host.shutdownSubscription(sub.subscriber_muid, sub.property_id, sub.res_id);
        std::cout << "After host shutdown - host subscriptions: " << host.getSubscriptions().size() << ", client subscriptions: " << client.getSubscriptions().size() << std::endl;
        EXPECT_EQ(0, client.getSubscriptions().size()) << "Client subscriptions not cleared after host shutdown";
        EXPECT_EQ(0, host.getSubscriptions().size()) << "Host subscriptions not cleared after shutdown";
    }
}

TEST(PropertyFacadesTest, propertyExchange2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    
    device1.sendDiscovery();
    
    auto connections = device1.getConnections();
    ASSERT_GT(connections.size(), 0) << "No connections established";
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";
    
    auto& client = conn->getPropertyClientFacade();
    client.sendGetPropertyData(PropertyResourceNames::CHANNEL_LIST, "", "");
}
