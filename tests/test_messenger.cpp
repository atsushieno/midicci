#include <gtest/gtest.h>
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/messages/Message.hpp"

using namespace midi_ci;

class MessengerTest : public ::testing::Test {
protected:
    void SetUp() override {
        device = std::make_unique<core::MidiCIDevice>();
        device->initialize();
    }
    
    std::unique_ptr<core::MidiCIDevice> device{nullptr};
};

TEST_F(MessengerTest, DeviceInitialization) {
    EXPECT_TRUE(device->is_initialized());
    EXPECT_NE(device->get_muid(), 0);
}

TEST_F(MessengerTest, DeviceInfo) {
    auto device_info = device->get_device_info();
    EXPECT_FALSE(device_info.manufacturer.empty());
}
