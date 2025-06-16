#include <gtest/gtest.h>
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/profiles/ProfileHostFacade.hpp"

using namespace midi_ci;

class ProfileFacadesTest : public ::testing::Test {
protected:
    void SetUp() override {
        device1 = std::make_unique<core::MidiCIDevice>(19474 & 0x7F7F7F7F);
        device2 = std::make_unique<core::MidiCIDevice>(37564 & 0x7F7F7F7F);
        
        device1->initialize();
        device2->initialize();
        
        device1->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
            device2->processInput(group, data);
            return true;
        });
        
        device2->set_sysex_sender([this](uint8_t group, const std::vector<uint8_t>& data) -> bool {
            device1->processInput(group, data);
            return true;
        });
    }
    
    std::unique_ptr<core::MidiCIDevice> device1;
    std::unique_ptr<core::MidiCIDevice> device2;
};

TEST_F(ProfileFacadesTest, configureProfiles) {
    auto& profile_host = device2->get_profile_host_facade();
    
    device1->sendDiscovery();
    
    auto connections = device1->get_connections();
    EXPECT_GT(connections.size(), 0);
}
