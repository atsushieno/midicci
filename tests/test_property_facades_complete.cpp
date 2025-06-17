#include <gtest/gtest.h>
#include "TestCIMediator.hpp"

TEST(PropertyFacadesCompleteTest, propertyExchange1) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}

TEST(PropertyFacadesCompleteTest, propertyExchange2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}
