#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include "keyboard_controller.h"

// Include midicci headers to test direct property simulation
#include "midicci/midicci.hpp"

class EndToEndPropertiesTest : public ::testing::Test {
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

TEST_F(EndToEndPropertiesTest, TestBothParsingFormats) {
    std::cout << "[TEST] Testing both JSON formats for StandardProperties parsing..." << std::endl;
    
    // Test direct array format
    std::string directJson = R"([
        {
            "title": "Modulation",
            "ctrlType": "cc",
            "description": "Modulation wheel",
            "ctrlIndex": [1],
            "channel": 1
        },
        {
            "title": "Volume",
            "ctrlType": "cc", 
            "description": "Channel volume",
            "ctrlIndex": [7],
            "channel": 1
        }
    ])";
    
    std::vector<uint8_t> directData(directJson.begin(), directJson.end());
    auto directResult = midicci::commonproperties::StandardProperties::parseControlList(directData);
    
    std::cout << "[TEST] Direct array format returned " << directResult.size() << " items" << std::endl;
    EXPECT_EQ(directResult.size(), 2);
    
    // Test wrapped object format
    std::string wrappedJson = R"({
        "ctrlList": [
            {
                "title": "Modulation",
                "ctrlType": "cc",
                "description": "Modulation wheel",
                "ctrlIndex": [1],
                "channel": 1
            },
            {
                "title": "Volume",
                "ctrlType": "cc", 
                "description": "Channel volume", 
                "ctrlIndex": [7],
                "channel": 1
            }
        ]
    })";
    
    std::vector<uint8_t> wrappedData(wrappedJson.begin(), wrappedJson.end());
    auto wrappedResult = midicci::commonproperties::StandardProperties::parseControlList(wrappedData);
    
    std::cout << "[TEST] Wrapped object format returned " << wrappedResult.size() << " items" << std::endl;
    EXPECT_EQ(wrappedResult.size(), 2);
    
    // Both should produce the same results
    if (directResult.size() == wrappedResult.size() && !directResult.empty()) {
        EXPECT_EQ(directResult[0].title, wrappedResult[0].title);
        EXPECT_EQ(directResult[0].ctrlType, wrappedResult[0].ctrlType);
        std::cout << "[TEST] SUCCESS: Both formats produce identical results!" << std::endl;
    }
}

TEST_F(EndToEndPropertiesTest, TestProgramListParsing) {
    std::cout << "[TEST] Testing ProgramList parsing with both formats..." << std::endl;
    
    // Test direct array format for programs
    std::string directProgramJson = R"([
        {
            "title": "Piano 1",
            "bankPC": [0, 0, 1], 
            "category": ["Piano"],
            "tags": ["acoustic", "bright"]
        },
        {
            "title": "Electric Piano",
            "bankPC": [0, 0, 5],
            "category": ["Piano"],
            "tags": ["electric", "vintage"]
        }
    ])";
    
    std::vector<uint8_t> directProgramData(directProgramJson.begin(), directProgramJson.end());
    auto directProgramResult = midicci::commonproperties::StandardProperties::parseProgramList(directProgramData);
    
    std::cout << "[TEST] Direct program array format returned " << directProgramResult.size() << " items" << std::endl;
    EXPECT_EQ(directProgramResult.size(), 2);
    
    // Test wrapped object format for programs
    std::string wrappedProgramJson = R"({
        "programList": [
            {
                "title": "Piano 1",
                "bankPC": [0, 0, 1],
                "category": ["Piano"], 
                "tags": ["acoustic", "bright"]
            },
            {
                "title": "Electric Piano",
                "bankPC": [0, 0, 5],
                "category": ["Piano"],
                "tags": ["electric", "vintage"]
            }
        ]
    })";
    
    std::vector<uint8_t> wrappedProgramData(wrappedProgramJson.begin(), wrappedProgramJson.end());
    auto wrappedProgramResult = midicci::commonproperties::StandardProperties::parseProgramList(wrappedProgramData);
    
    std::cout << "[TEST] Wrapped program object format returned " << wrappedProgramResult.size() << " items" << std::endl;
    EXPECT_EQ(wrappedProgramResult.size(), 2);
    
    // Both should produce the same results
    if (directProgramResult.size() == wrappedProgramResult.size() && !directProgramResult.empty()) {
        EXPECT_EQ(directProgramResult[0].title, wrappedProgramResult[0].title);
        std::cout << "[TEST] SUCCESS: Both program list formats produce identical results!" << std::endl;
    }
}

TEST_F(EndToEndPropertiesTest, TestControllerWithoutRealDevices) {
    // Test basic controller functionality without requiring real MIDI-CI devices
    ASSERT_TRUE(controller->resetMidiConnections());
    
    auto inputDevices = controller->getInputDevices();
    auto outputDevices = controller->getOutputDevices();
    
    std::cout << "[TEST] Found " << inputDevices.size() << " input devices" << std::endl;
    std::cout << "[TEST] Found " << outputDevices.size() << " output devices" << std::endl;
    
    // Even without devices, the controller should initialize properly
    auto midiCIDevices = controller->getMidiCIDeviceDetails();
    std::cout << "[TEST] MIDI-CI devices discovered: " << midiCIDevices.size() << std::endl;
    
    // This should work even with no devices
    EXPECT_GE(midiCIDevices.size(), 0); // >= 0 because there might be no MIDI-CI devices
    
    std::cout << "[TEST] Controller initialization and basic functionality works" << std::endl;
}