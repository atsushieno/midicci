#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include <midicci/midicci.hpp>  // was: midicci/ClientConnection.hpp"

TEST(MidiCIDeviceTest, initialState) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    
    EXPECT_EQ(19474, device1.get_muid());
}

TEST(MidiCIDeviceTest, basicRun) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    device1.sendDiscovery();
    EXPECT_EQ(1, device1.get_connections().size()) << "connections.size";
    
    auto connections = device1.get_connections();
    auto it = connections.find(device2.get_muid());
    ASSERT_NE(connections.end(), it) << "conn";
    auto conn = it->second;
    ASSERT_NE(nullptr, conn) << "conn";

    ASSERT_NE(nullptr, conn->get_device_info()) << "conn->get_device_info()";
}
