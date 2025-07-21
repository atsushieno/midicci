/*
#include <gtest/gtest.h>
#include <midicci/midicci.hpp>  // was: midicci/core/MidiCIDevice.hpp"
#include <midicci/midicci.hpp>  // was: midicci/messages/Messenger.hpp"
#include <midicci/midicci.hpp>  // was: midicci/messages/Message.hpp"
#include <midicci/midicci.hpp>  // was: midicci/core/MidiCIDeviceConfiguration.hpp"

using namespace midi_ci;

class MessengerTest : public ::testing::Test {
protected:
    void SetUp() override {
        device = std::make_unique<core::MidiCIDevice>(0x12345678, config);
        device->initialize();
    }
    
    std::unique_ptr<core::MidiCIDevice> device{nullptr};
    midi_ci::core::DeviceConfig config{};
};

TEST_F(MessengerTest, DeviceInitialization) {
    EXPECT_TRUE(device->is_initialized());
    EXPECT_NE(device->get_muid(), 0);
}

TEST_F(MessengerTest, DeviceInfo) {
    auto device_info = device->get_device_info();
    EXPECT_NE(device_info.manufacturer, 0);
}

TEST_F(MessengerTest, ProcessInputMidi1Format) {
    messages::Messenger messenger(*device);
    
    std::vector<uint8_t> midi1_data = {
        0x7E, 0x7F, 0x0D, 0x70, 0x01,
        0x10, 0x10, 0x10, 0x10, 0x7F, 0x7F, 0x7F, 0x7F,
        0x56, 0x34, 0x12, 0x57, 0x13, 0x68, 0x24,
        0x7F, 0x5F, 0x3F, 0x1F, 0x1C,
        0x00, 0x02, 0x00, 0x00, 0x00
    };
    
    EXPECT_NO_THROW(messenger.process_input(0, midi1_data));
}

TEST_F(MessengerTest, ProcessInputMidi2Format) {
    messages::Messenger messenger(*device);
    
    std::vector<uint8_t> midi2_data = {
        0x7E, 0x7F, 0x0D, 0x70, 0x01,
        0x10, 0x10, 0x10, 0x10, 0x7F, 0x7F, 0x7F, 0x7F,
        0x56, 0x34, 0x12, 0x57, 0x13, 0x68, 0x24,
        0x7F, 0x5F, 0x3F, 0x1F, 0x1C,
        0x00, 0x02, 0x00, 0x00, 0x00
    };
    
    EXPECT_NO_THROW(messenger.process_input(0, midi2_data));
}
*/
