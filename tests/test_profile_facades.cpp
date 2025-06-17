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
    
    device1.sendDiscovery();
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0) << "No connections established after discovery";
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn) << "Connection is null";

    EXPECT_EQ(0, conn->get_profile_client_facade().get_profiles().get_profiles().size()) << "profiles.size before addProfile";

    MidiCIProfile localProfile(
        MidiCIProfileId({1, 2, 3, 4, 5}),
        0, 1, true, 1
    );
    device2.get_profile_host_facade().add_profile(localProfile);
    EXPECT_EQ(1, conn->get_profile_client_facade().get_profiles().get_profiles().size()) << "profiles.size after addProfile";

    auto& remoteProfiles = conn->get_profile_client_facade().get_profiles().get_profiles();
    auto remoteProfile = remoteProfiles[0];
    EXPECT_EQ(localProfile.profile.to_string(), remoteProfile.profile.to_string()) << "localProfile == remoteProfile: profile";
    EXPECT_EQ(localProfile.address, remoteProfile.address) << "localProfile == remoteProfile: address";
    EXPECT_TRUE(localProfile.enabled) << "localProfile.enabled";
    EXPECT_TRUE(remoteProfile.enabled) << "remoteProfile.enabled";
    EXPECT_EQ(0, remoteProfile.group) << "remoteProfile.group";

    bool profileEnabledRemotely = false;
    bool profileDisabledRemotely = false;
    device2.set_profile_set_callback([&](const MidiCIProfile& profile) {
        if (profile.enabled)
            profileEnabledRemotely = true;
        else
            profileDisabledRemotely = true;
    });

    device2.get_profile_host_facade().disable_profile(localProfile.group, localProfile.address, localProfile.profile, 1);
    EXPECT_TRUE(profileDisabledRemotely) << "profileDisabledRemotely";
    EXPECT_FALSE(profileEnabledRemotely) << "profileEnabledRemotely";

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
    EXPECT_EQ(2, numEnabledProfiles) << "numEnabledProfiles";
    EXPECT_EQ(1, numDisabledProfiles) << "numDisabledProfiles";

    std::vector<uint8_t> profileDetailsData = {1, 3, 5, 7, 9};
    uint8_t detailsTarget = 0x40;
    device2.get_profile_host_facade().add_profile_details(
        localProfile.profile, detailsTarget, profileDetailsData
    );
    
    auto retrievedDetails = device2.get_profile_host_facade().get_profile_details(
        localProfile.profile, detailsTarget
    );
    EXPECT_EQ(profileDetailsData, retrievedDetails) << "getLocalProfileDetails";

    bool detailsValidated = false;
    device1.set_message_received_callback([&](const auto& msg) {
        if (msg.get_type() == MessageType::ProfileDetailsReply) {
            auto* detailsReply = dynamic_cast<const ProfileDetailsReply*>(&msg);
            if (detailsReply) {
                EXPECT_EQ(profileDetailsData, detailsReply->get_data()) << "ProfileDetailsReply data";
                detailsValidated = true;
            }
        }
    });
    device1.request_profile_details(localProfile.address, device2.get_muid(), localProfile.profile, detailsTarget);
    EXPECT_TRUE(detailsValidated) << "detailsValidated";

    std::vector<uint8_t> profileSpecificData = {9, 8, 7, 6, 5};
    bool specificDataValidated = false;
    device2.set_message_received_callback([&](const auto& msg) {
        if (msg.get_type() == MessageType::ProfileSpecificData) {
            auto* specificMsg = dynamic_cast<const ProfileSpecificData*>(&msg);
            if (specificMsg) {
                EXPECT_EQ(profileSpecificData, specificMsg->get_data()) << "ProfileSpecificData";
                specificDataValidated = true;
            }
        }
    });
    device1.send_profile_specific_data(localProfile.address, device2.get_muid(), localProfile.profile, profileSpecificData);
    EXPECT_TRUE(specificDataValidated) << "specificDataValidated";
}

TEST(ProfileFacadesTest, configureProfiles3) {
    TestCIMediator mediator;
    auto& device1 = mediator.getDevice1();
    device1.get_config().set_auto_send_profile_inquiry(false);
    auto& device2 = mediator.getDevice2();

    MidiCIProfile localProfile(MidiCIProfileId({1, 2, 3, 4, 5}), 0, 1, true, 1);
    device2.get_profile_host_facade().add_profile(localProfile);

    device1.sendDiscovery();
    auto connections = device1.get_connections();
    ASSERT_GT(connections.size(), 0);
    auto conn = connections.begin()->second;
    ASSERT_NE(nullptr, conn);

    EXPECT_EQ(0, conn->get_profile_client_facade().get_profiles().get_profiles().size()) << "profiles.size before requesting";

    device1.request_profiles(0, MidiCIConstants::ADDRESS_FUNCTION_BLOCK, conn->get_target_muid());
    EXPECT_EQ(1, conn->get_profile_client_facade().get_profiles().get_profiles().size()) << "profiles.size after requesting";
}
