#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include <midicci/midicci.hpp>

using namespace midicci;

TEST(ProfileFacadesTest, configureProfiles) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    MidiCIProfile localProfile(
        MidiCIProfileId({1, 2, 3, 4, 5}),
        0, 1, true, 1
    );
    device2.getProfileHostFacade().addProfile(localProfile);

    device1.sendDiscovery();
    auto connections = device1.getConnections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";

    EXPECT_EQ(1, conn->getProfileClientFacade().getProfiles().getProfiles().size()) << "profilecommonrules.size after addProfile";

    auto& remoteProfiles = conn->getProfileClientFacade().getProfiles().getProfiles();
    ASSERT_GT(remoteProfiles.size(), 0) << "No entry in remoteProfiles";
    auto remoteProfile = remoteProfiles[0];
    EXPECT_EQ(localProfile.profile.toString(), remoteProfile.profile.toString()) << "localProfile == remoteProfile: profile";
    EXPECT_EQ(localProfile.address, remoteProfile.address) << "localProfile == remoteProfile: address";
    EXPECT_TRUE(localProfile.enabled) << "localProfile.enabled";
    EXPECT_TRUE(remoteProfile.enabled) << "remoteProfile.enabled";
    EXPECT_EQ(0, remoteProfile.group) << "remoteProfile.group";

    device2.getProfileHostFacade().disableProfile(localProfile.group, localProfile.address, localProfile.profile, 1);

    auto& localProfilesUpdated = device2.getProfileHostFacade().getProfiles().getProfiles();
    auto localProfileUpdated = localProfilesUpdated[0];
    EXPECT_FALSE(localProfileUpdated.enabled) << "local profile is disabled";
}

TEST(ProfileFacadesTest, configureProfiles2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    int numEnabledProfiles = 0;
    int numDisabledProfiles = 0;
    device1.setMessageReceivedCallback([&](const auto& msg) {
        if (msg.getType() == MessageType::ProfileInquiryReply) {
            auto* profileReply = dynamic_cast<const ProfileReply*>(&msg);
            if (profileReply) {
                numEnabledProfiles = profileReply->getEnabledProfiles().size();
                numDisabledProfiles = profileReply->getDisabledProfiles().size();
            }
        }
    });

    MidiCIProfile localProfile(MidiCIProfileId({1, 2, 3, 4, 5}), 0, 1, true, 1);
    MidiCIProfile localProfile2(MidiCIProfileId({2, 3, 4, 5, 6}), 0, 1, true, 1);
    MidiCIProfile localProfile3(MidiCIProfileId({3, 4, 5, 6, 7}), 0, 1, false, 1);
    
    device2.getProfileHostFacade().addProfile(localProfile);
    device2.getProfileHostFacade().addProfile(localProfile2);
    device2.getProfileHostFacade().addProfile(localProfile3);

    device1.sendDiscovery();
    EXPECT_EQ(2, numEnabledProfiles) << "numEnabledProfiles";
    EXPECT_EQ(1, numDisabledProfiles) << "numDisabledProfiles";


}

TEST(ProfileFacadesTest, configureProfiles3) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();

    MidiCIProfile localProfile(MidiCIProfileId({1, 2, 3, 4, 5}), 0, 1, true, 1);
    device2.getProfileHostFacade().addProfile(localProfile);

    device1.sendDiscovery();
    auto connections = device1.getConnections();
    ASSERT_GT(connections.size(), 0);
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    EXPECT_EQ(1, conn->getProfileClientFacade().getProfiles().getProfiles().size()) << "profilecommonrules.size after requesting";
}
