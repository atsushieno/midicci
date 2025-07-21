/*
#include <gtest/gtest.h>
#include <midicci/midicci.hpp>  // was: midicci/core/MidiCIDevice.hpp"
#include <midicci/midicci.hpp>  // was: midicci/messages/Message.hpp"

using namespace midi_ci;

class MidiCIDeviceExtendedTest : public ::testing::Test {
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

TEST_F(MidiCIDeviceExtendedTest, initialState) {
    auto device_info = device1->get_device_info();
    EXPECT_EQ(19474, device1->get_muid());
    EXPECT_FALSE(device_info.manufacturer.empty());
}

TEST_F(MidiCIDeviceExtendedTest, basicRun) {
    device1->sendDiscovery();

    auto connections = device1->get_connections();
    EXPECT_EQ(1, connections.size());

    auto conn = device1->get_connection(device2->get_muid() & 0xFF);
    EXPECT_NE(nullptr, conn);
}
*/