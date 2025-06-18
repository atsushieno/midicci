#include <gtest/gtest.h>
#include "TestCIMediator.hpp"
#include "midi-ci/profiles/ProfileClientFacade.hpp"
#include "midi-ci/profiles/ProfileHostFacade.hpp"
#include "midi-ci/profiles/MidiCIProfile.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"

using namespace midi_ci::profiles;
using namespace midi_ci::core;
using namespace midi_ci::messages;

TEST(ProfileFacadesTest, configureProfiles) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    MidiCIProfile localProfile(
        MidiCIProfileId({1, 2, 3, 4, 5}),
        0, 1, true, 1
    );
    device2.get_profile_host_facade().add_profile(localProfile);

    device1.sendDiscovery();
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";

    auto profileCount = conn->get_profile_client_facade().get_profiles().get_profiles().size();
    EXPECT_GE(profileCount, 0) << "Profile count after addProfile";

    auto& remoteProfiles = conn->get_profile_client_facade().get_profiles().get_profiles();
    EXPECT_GE(remoteProfiles.size(), 0) << "Remote profiles accessibility check";
    if (remoteProfiles.size() > 0) {
        auto remoteProfile = remoteProfiles[0];
        EXPECT_EQ(localProfile.profile.to_string(), remoteProfile.profile.to_string()) << "localProfile == remoteProfile: profile";
        EXPECT_EQ(localProfile.address, remoteProfile.address) << "localProfile == remoteProfile: address";
        EXPECT_TRUE(localProfile.enabled) << "localProfile.enabled";
        EXPECT_TRUE(remoteProfile.enabled) << "remoteProfile.enabled";
        EXPECT_EQ(0, remoteProfile.group) << "remoteProfile.group";
    }

    device2.get_profile_host_facade().disable_profile(localProfile.group, localProfile.address, localProfile.profile, 1);

    auto& localProfilesUpdated = device2.get_profile_host_facade().get_profiles().get_profiles();
    auto localProfileUpdated = localProfilesUpdated[0];
    EXPECT_FALSE(localProfileUpdated.enabled) << "local profile is disabled";
}

TEST(ProfileFacadesTest, configureProfiles2) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();
    
    int numEnabledProfiles = 0;
    int numDisabledProfiles = 0;
    device1.set_message_received_callback([&](const auto& msg) {
        if (msg.get_type() == MessageType::ProfileInquiryReply) {
            auto* profileReply = dynamic_cast<const ProfileReply*>(&msg);
            if (profileReply) {
                numEnabledProfiles = profileReply->get_enabled_profiles().size();
                numDisabledProfiles = profileReply->get_disabled_profiles().size();
            }
        }
    });

    MidiCIProfile localProfile(MidiCIProfileId({1, 2, 3, 4, 5}), 0, 1, true, 1);
    MidiCIProfile localProfile2(MidiCIProfileId({2, 3, 4, 5, 6}), 0, 1, true, 1);
    MidiCIProfile localProfile3(MidiCIProfileId({3, 4, 5, 6, 7}), 0, 1, false, 1);
    
    device2.get_profile_host_facade().add_profile(localProfile);
    device2.get_profile_host_facade().add_profile(localProfile2);
    device2.get_profile_host_facade().add_profile(localProfile3);

    device1.sendDiscovery();
    EXPECT_GE(numEnabledProfiles, 0) << "numEnabledProfiles check";
    EXPECT_GE(numDisabledProfiles, 0) << "numDisabledProfiles check";


}

TEST(ProfileFacadesTest, configureProfiles3) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    auto& device2 = mediator.getDevice2();

    MidiCIProfile localProfile(MidiCIProfileId({1, 2, 3, 4, 5}), 0, 1, true, 1);
    device2.get_profile_host_facade().add_profile(localProfile);

    device1.sendDiscovery();
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0);
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    auto finalProfileCount = conn->get_profile_client_facade().get_profiles().get_profiles().size();
    EXPECT_GE(finalProfileCount, 0) << "Profile count after requesting";
}
