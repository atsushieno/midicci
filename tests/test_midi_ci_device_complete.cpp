#include <gtest/gtest.h>
#include "TestCIMediator.hpp"

TEST(MidiCIDeviceCompleteTest, initialState) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}

TEST(MidiCIDeviceCompleteTest, basicRun) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
    
    device1.sendDiscovery();
    device2.sendDiscovery();
}
