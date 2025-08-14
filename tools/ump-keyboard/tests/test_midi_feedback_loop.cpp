#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include "keyboard_controller.h"

class MIDIFeedbackLoopTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_unique<KeyboardController>();
    }
    
    void TearDown() override {
        if (controller) {
            controller.reset();
        }
    }
    
    std::unique_ptr<KeyboardController> controller;
};

TEST_F(MIDIFeedbackLoopTest, TestGUIDeviceSelection) {
    // Reset MIDI connections to initialize the system
    ASSERT_TRUE(controller->resetMidiConnections());
    
    // Get available input devices
    auto inputDevices = controller->getInputDevices();
    std::cout << "[TEST] Found " << inputDevices.size() << " input devices" << std::endl;
    
    // Get available output devices  
    auto outputDevices = controller->getOutputDevices();
    std::cout << "[TEST] Found " << outputDevices.size() << " output devices" << std::endl;
    
    // If we have devices available, select first input and output
    if (!inputDevices.empty()) {
        std::cout << "[TEST] Selecting first input device: " << inputDevices[0].second << std::endl;
        EXPECT_TRUE(controller->selectInputDevice(inputDevices[0].first));
    }
    
    if (!outputDevices.empty()) {
        std::cout << "[TEST] Selecting first output device: " << outputDevices[0].second << std::endl;
        EXPECT_TRUE(controller->selectOutputDevice(outputDevices[0].first));
    }
    
    // Wait a moment to observe any immediate feedback loops
    std::cout << "[TEST] Waiting 2 seconds to observe logs for infinite loops..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Test sending MIDI-CI discovery if both devices are connected
    if (!inputDevices.empty() && !outputDevices.empty()) {
        std::cout << "[TEST] Testing MIDI-CI discovery with connected devices..." << std::endl;
        controller->sendMidiCIDiscovery();
        
        // Wait to see if discovery triggers feedback loops
        std::cout << "[TEST] Waiting 3 seconds after discovery to observe feedback loops..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
    }
    
    std::cout << "[TEST] Test completed. Check logs above for '[SYSEX DETECTED]' and 'Continue packet' patterns." << std::endl;
    std::cout << "[TEST] If buffer sizes keep increasing, that indicates an infinite loop." << std::endl;
}

TEST_F(MIDIFeedbackLoopTest, TestDeviceRefresh) {
    // Test refreshing devices - this should not cause loops
    std::cout << "[TEST] Testing device refresh functionality..." << std::endl;
    
    ASSERT_TRUE(controller->resetMidiConnections());
    controller->refreshDevices();
    
    // Wait to observe any side effects
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    std::cout << "[TEST] Device refresh completed" << std::endl;
}

TEST_F(MIDIFeedbackLoopTest, TestNoteOperations) {
    // Test basic MIDI note operations with device selection
    ASSERT_TRUE(controller->resetMidiConnections());
    
    auto inputDevices = controller->getInputDevices();
    auto outputDevices = controller->getOutputDevices();
    
    if (!inputDevices.empty() && !outputDevices.empty()) {
        controller->selectInputDevice(inputDevices[0].first);
        controller->selectOutputDevice(outputDevices[0].first);
        
        std::cout << "[TEST] Testing note operations..." << std::endl;
        
        // Send some notes
        controller->noteOn(60, 100);  // Middle C
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        controller->noteOff(60);
        
        // Wait to see if note operations trigger any feedback
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "[TEST] Note operations completed" << std::endl;
    } else {
        std::cout << "[TEST] Skipping note operations - no devices available" << std::endl;
    }
}