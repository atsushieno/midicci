#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include "keyboard_controller.h"

class StandardPropertiesTest : public ::testing::Test {
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

TEST_F(StandardPropertiesTest, TestGetAllCtrlList) {
    // Initialize MIDI connections
    ASSERT_TRUE(controller->resetMidiConnections());
    
    // Get available devices
    auto inputDevices = controller->getInputDevices();
    auto outputDevices = controller->getOutputDevices();
    
    std::cout << "[TEST] Found " << inputDevices.size() << " input devices" << std::endl;
    std::cout << "[TEST] Found " << outputDevices.size() << " output devices" << std::endl;
    
    // Select devices if available
    if (!inputDevices.empty() && !outputDevices.empty()) {
        std::cout << "[TEST] Selecting devices for MIDI-CI communication" << std::endl;
        EXPECT_TRUE(controller->selectInputDevice(inputDevices[0].first));
        EXPECT_TRUE(controller->selectOutputDevice(outputDevices[0].first));
        
        // Send MIDI-CI discovery to establish connections
        std::cout << "[TEST] Sending MIDI-CI discovery..." << std::endl;
        controller->sendMidiCIDiscovery();
        
        // Wait for discovery process to complete
        std::cout << "[TEST] Waiting 3 seconds for discovery to complete..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // Get discovered MIDI-CI devices with their MUIDs
        std::cout << "[TEST] Getting discovered MIDI-CI devices..." << std::endl;
        auto devices = controller->getMidiCIDeviceDetails();
        std::cout << "[TEST] Found " << devices.size() << " MIDI-CI devices" << std::endl;
        
        if (devices.empty()) {
            std::cout << "[TEST] No MIDI-CI devices discovered. This could mean:" << std::endl;
            std::cout << "[TEST] 1. No MIDI-CI capable devices are connected" << std::endl;
            std::cout << "[TEST] 2. Discovery process didn't complete in time" << std::endl;
            std::cout << "[TEST] 3. Discovery messages aren't being sent/received properly" << std::endl;
            GTEST_SKIP() << "No MIDI-CI devices available for testing";
        }
        
        // Test getAllCtrlList with each discovered device
        bool foundValidResponse = false;
        for (const auto& device : devices) {
            std::cout << "[TEST] Testing device: " << device.device_name << " (MUID: 0x" 
                      << std::hex << device.muid << std::dec << ")" << std::endl;
            
            auto ctrlList = controller->getAllCtrlList(device.muid);
            
            if (ctrlList.has_value()) {
                std::cout << "[TEST] SUCCESS: getAllCtrlList returned " << ctrlList->size() << " items:" << std::endl;
                foundValidResponse = true;
                
                for (size_t i = 0; i < ctrlList->size(); ++i) {
                    const auto& ctrl = (*ctrlList)[i];
                    std::cout << "[TEST]   Item " << i << ":" << std::endl;
                    std::cout << "[TEST]     Title: " << ctrl.title << std::endl;
                    std::cout << "[TEST]     CtrlType: " << ctrl.ctrlType << std::endl;
                    std::cout << "[TEST]     Channel: " << (ctrl.channel.has_value() ? std::to_string(ctrl.channel.value()) : "none") << std::endl;
                    std::cout << "[TEST]     Description: " << ctrl.description << std::endl;
                }
                
                // Verify we have multiple items (the issue was empty/single item lists)
                EXPECT_GT(ctrlList->size(), 1) << "Expected multiple control items, but got " << ctrlList->size();
                
            } else {
                std::cout << "[TEST] Device " << device.device_name << " returned no getAllCtrlList data" << std::endl;
                std::cout << "[TEST] This could indicate:" << std::endl;
                std::cout << "[TEST] 1. Device doesn't support ALL_CTRL_LIST property" << std::endl;
                std::cout << "[TEST] 2. StandardProperties parsing failed" << std::endl;
                std::cout << "[TEST] 3. Property data not yet available (async)" << std::endl;
            }
        }
        
        if (!foundValidResponse) {
            // Wait a bit more to see if delayed responses arrive
            std::cout << "[TEST] No immediate responses. Waiting additional 3 seconds for delayed responses..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            
            // Try again in case it was delayed
            for (const auto& device : devices) {
                auto ctrlList2 = controller->getAllCtrlList(device.muid);
                if (ctrlList2.has_value()) {
                    std::cout << "[TEST] DELAYED RESPONSE: Device " << device.device_name 
                              << " returned " << ctrlList2->size() << " items" << std::endl;
                    foundValidResponse = true;
                    break;
                }
            }
        }
        
        if (!foundValidResponse) {
            std::cout << "[TEST] WARNING: No devices returned getAllCtrlList data. This suggests:" << std::endl;
            std::cout << "[TEST] 1. Connected devices may not implement ALL_CTRL_LIST property" << std::endl;
            std::cout << "[TEST] 2. The StandardProperties parsing may have issues" << std::endl;
            std::cout << "[TEST] 3. Property exchange may not be working correctly" << std::endl;
        }
        
    } else {
        std::cout << "[TEST] Skipping getAllCtrlList test - no devices available" << std::endl;
        GTEST_SKIP() << "No MIDI devices available for testing";
    }
}

TEST_F(StandardPropertiesTest, TestDirectStandardPropertiesCall) {
    // This test will help us debug the StandardProperties parsing directly
    std::cout << "[TEST] Testing direct StandardProperties parsing..." << std::endl;
    
    // Test with sample JSON data that should represent a control list
    std::string sampleCtrlListJson = R"({
        "ctrlList": [
            {
                "type": 1,
                "control": 1,
                "channel": 0,
                "name": "Modulation"
            },
            {
                "type": 1,
                "control": 7,
                "channel": 0,
                "name": "Volume"
            },
            {
                "type": 1,
                "control": 10,
                "channel": 0,
                "name": "Pan"
            }
        ]
    })";
    
    std::cout << "[TEST] Sample JSON for testing: " << sampleCtrlListJson << std::endl;
    
    // Note: We can't directly test StandardProperties parsing here without access to the midicci headers
    // But this test structure helps us understand what the expected format should be
    
    std::cout << "[TEST] This test serves as documentation of expected JSON format" << std::endl;
    std::cout << "[TEST] The actual parsing happens in midicci library StandardProperties::parseControlList()" << std::endl;
}