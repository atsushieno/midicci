/*
#include <gtest/gtest.h>
#include <midicci/midicci.hpp>  // was: midicci/profiles/ProfileManager.hpp"

using namespace midi_ci::profiles;

class ProfileManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<ProfileManager>();
        profile_id = ProfileId({0x7E, 0x00, 0x01, 0x02, 0x03});
        profile = Profile(profile_id, 0, 0x7F, false, 16);
    }
    
    std::unique_ptr<ProfileManager> manager{nullptr};
    ProfileId profile_id{{0x7E, 0x00, 0x01, 0x02, 0x03}};
    Profile profile{ProfileId{{0x7E, 0x00, 0x01, 0x02, 0x03}}, 0, 0x7F, false, 16};
};

TEST_F(ProfileManagerTest, AddProfile) {
    manager->add_profile(profile);
    
    auto profiles = manager->get_profiles();
    EXPECT_EQ(profiles.size(), 1);
    EXPECT_EQ(profiles[0].profile_id, profile_id);
}

TEST_F(ProfileManagerTest, EnableProfile) {
    manager->add_profile(profile);
    manager->enable_profile(0, 0x7F, profile_id, 16);
    
    auto enabled_profiles = manager->get_enabled_profiles();
    EXPECT_EQ(enabled_profiles.size(), 1);
    EXPECT_TRUE(enabled_profiles[0].enabled);
}

TEST_F(ProfileManagerTest, DisableProfile) {
    manager->add_profile(profile);
    manager->enable_profile(0, 0x7F, profile_id, 16);
    manager->disable_profile(0, 0x7F, profile_id);
    
    auto disabled_profiles = manager->get_disabled_profiles();
    EXPECT_EQ(disabled_profiles.size(), 1);
    EXPECT_FALSE(disabled_profiles[0].enabled);
}

TEST_F(ProfileManagerTest, FindProfile) {
    manager->add_profile(profile);
    
    auto found_profile = manager->find_profile(0, 0x7F, profile_id);
    ASSERT_NE(found_profile, nullptr);
    EXPECT_EQ(found_profile->profile_id, profile_id);
}
*/
