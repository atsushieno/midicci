#include <gtest/gtest.h>
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/properties/PropertyHostFacade.hpp"

using namespace midi_ci;

class PropertyFacadesTest : public ::testing::Test {
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

TEST_F(PropertyFacadesTest, propertyExchange1) {
    auto& host = device2->get_property_host_facade();
    
    device1->sendDiscovery();
    
    auto connections = device1->get_connections();
    EXPECT_GT(connections.size(), 0);
}
