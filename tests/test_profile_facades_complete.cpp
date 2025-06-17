#include <gtest/gtest.h>
#include "TestCIMediator.hpp"

TEST(ProfileFacadesCompleteTest, configureProfiles) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    std::vector<uint8_t> profileId = {0x7E, 0x40, 0x01, 0x02, 0x03};
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}

TEST(ProfileFacadesCompleteTest, configureProfiles2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    std::vector<uint8_t> profileId = {0x7E, 0x40, 0x01, 0x02, 0x03};
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}

TEST(ProfileFacadesCompleteTest, configureProfiles3) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    std::vector<uint8_t> profileId = {0x7E, 0x40, 0x01, 0x02, 0x03};
    
    EXPECT_EQ(19474 & 0x7F7F7F7F, device1.get_muid());
    EXPECT_EQ(37564 & 0x7F7F7F7F, device2.get_muid());
}
