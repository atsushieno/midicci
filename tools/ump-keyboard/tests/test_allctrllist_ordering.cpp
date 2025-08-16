#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <unordered_set>
#include "keyboard_controller.h"
#include "midicci/midicci.hpp"

class AllCtrlListOrderingTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_unique<KeyboardController>();
        
        // Set up MIDI-CI properties changed callback to track when properties are updated
        controller->setMidiCIPropertiesChangedCallback([this](uint32_t muid) {
            std::cout << "[TEST-CALLBACK] Properties updated for MUID: 0x" << std::hex << muid << std::dec << std::endl;
            properties_updated_muids.insert(muid);
        });
    }
    
    void TearDown() override {
        if (controller) {
            controller.reset();
        }
    }
    
    std::unique_ptr<KeyboardController> controller;
    std::unordered_set<uint32_t> properties_updated_muids;
    
    // Helper function to find matching input/output device pairs
    std::vector<std::pair<std::string, std::string>> findMatchingDevicePairs() {
        std::vector<std::pair<std::string, std::string>> pairs;
        
        auto inputDevices = controller->getInputDevices();
        auto outputDevices = controller->getOutputDevices();
        
        std::cout << "[TEST] Found " << inputDevices.size() << " input devices:" << std::endl;
        for (const auto& device : inputDevices) {
            std::cout << "[TEST]   Input: " << device.second << " (" << device.first << ")" << std::endl;
        }
        
        std::cout << "[TEST] Found " << outputDevices.size() << " output devices:" << std::endl;
        for (const auto& device : outputDevices) {
            std::cout << "[TEST]   Output: " << device.second << " (" << device.first << ")" << std::endl;
        }
        
        // Find devices with identical names (indicating they're the same physical device)
        for (const auto& input : inputDevices) {
            for (const auto& output : outputDevices) {
                if (input.second == output.second) {
                    pairs.emplace_back(input.first, output.first);
                    std::cout << "[TEST] Found matching pair: " << input.second << std::endl;
                }
            }
        }
        
        return pairs;
    }
    
    // Helper function to verify control ordering by ctrlIndex
    bool verifyControlOrdering(const std::vector<midicci::commonproperties::MidiCIControl>& controls) {
        std::cout << "[TEST] Verifying control ordering for " << controls.size() << " controls..." << std::endl;
        
        if (controls.size() < 2) {
            std::cout << "[TEST] Not enough controls to verify ordering (need at least 2)" << std::endl;
            return true; // Trivially ordered
        }
        
        // Check if controls are ordered by ctrlIndex (ascending)
        bool isOrdered = true;
        std::vector<uint8_t> previousCtrlIndex;
        
        for (size_t i = 0; i < controls.size(); ++i) {
            const auto& ctrl = controls[i];
            
            std::cout << "[TEST]   Control " << i << ":" << std::endl;
            std::cout << "[TEST]     Title: '" << ctrl.title << "'" << std::endl;
            std::cout << "[TEST]     CtrlType: " << ctrl.ctrlType << std::endl;
            std::cout << "[TEST]     CtrlIndex: [";
            for (size_t j = 0; j < ctrl.ctrlIndex.size(); ++j) {
                if (j > 0) std::cout << ", ";
                std::cout << static_cast<int>(ctrl.ctrlIndex[j]);
            }
            std::cout << "]" << std::endl;
            std::cout << "[TEST]     Channel: " << (ctrl.channel.has_value() ? std::to_string(ctrl.channel.value()) : "none") << std::endl;
            std::cout << "[TEST]     Description: '" << ctrl.description << "'" << std::endl;
            
            // Check for blank titles (this was one of the reported issues)
            if (ctrl.title.empty()) {
                std::cout << "[TEST]     WARNING: Control has blank title!" << std::endl;
            }
            
            // Check ordering by comparing ctrlIndex
            if (i > 0) {
                // Compare ctrlIndex arrays lexicographically
                bool currentIsGreaterOrEqual = std::lexicographical_compare(
                    previousCtrlIndex.begin(), previousCtrlIndex.end(),
                    ctrl.ctrlIndex.begin(), ctrl.ctrlIndex.end()
                ) || previousCtrlIndex == ctrl.ctrlIndex;
                
                if (!currentIsGreaterOrEqual) {
                    std::cout << "[TEST]     ERROR: Control at index " << i 
                              << " has ctrlIndex that should come before previous control!" << std::endl;
                    isOrdered = false;
                }
            }
            
            previousCtrlIndex = ctrl.ctrlIndex;
        }
        
        return isOrdered;
    }
};

TEST_F(AllCtrlListOrderingTest, TestAllCtrlListOrdering) {
    std::cout << "[TEST] Starting AllCtrlList ordering verification test..." << std::endl;
    
    // Initialize MIDI connections
    ASSERT_TRUE(controller->resetMidiConnections()) << "Failed to initialize MIDI connections";
    
    // Find matching device pairs
    auto devicePairs = findMatchingDevicePairs();
    
    if (devicePairs.empty()) {
        std::cout << "[TEST] No matching input/output device pairs found." << std::endl;
        std::cout << "[TEST] This test requires devices with identical names for input and output." << std::endl;
        GTEST_SKIP() << "No matching MIDI device pairs available for MIDI-CI testing";
    }
    
    // Test with the first matching pair
    const auto& devicePair = devicePairs[0];
    std::cout << "[TEST] Using device pair - Input: " << devicePair.first 
              << ", Output: " << devicePair.second << std::endl;
    
    // Select the device pair
    ASSERT_TRUE(controller->selectInputDevice(devicePair.first)) 
        << "Failed to select input device: " << devicePair.first;
    ASSERT_TRUE(controller->selectOutputDevice(devicePair.second)) 
        << "Failed to select output device: " << devicePair.second;
    
    // Send MIDI-CI discovery to establish connections
    std::cout << "[TEST] Sending MIDI-CI discovery..." << std::endl;
    controller->sendMidiCIDiscovery();
    
    // Wait for discovery process to complete
    std::cout << "[TEST] Waiting 5 seconds for discovery to complete..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // Get discovered MIDI-CI devices
    auto devices = controller->getMidiCIDeviceDetails();
    std::cout << "[TEST] Found " << devices.size() << " MIDI-CI devices after discovery" << std::endl;
    
    if (devices.empty()) {
        std::cout << "[TEST] No MIDI-CI devices discovered. Possible reasons:" << std::endl;
        std::cout << "[TEST] 1. Connected devices don't support MIDI-CI" << std::endl;
        std::cout << "[TEST] 2. Discovery messages aren't being transmitted properly" << std::endl;
        std::cout << "[TEST] 3. Device loopback isn't configured correctly" << std::endl;
        GTEST_SKIP() << "No MIDI-CI devices discovered for testing";
    }
    
    // Test AllCtrlList ordering for each discovered device
    bool foundValidControlList = false;
    bool allControlListsOrdered = true;
    
    for (const auto& device : devices) {
        std::cout << "\n[TEST] ========================================" << std::endl;
        std::cout << "[TEST] Testing device: " << device.device_name 
                  << " (MUID: 0x" << std::hex << device.muid << std::dec << ")" << std::endl;
        std::cout << "[TEST] Manufacturer: " << device.manufacturer << std::endl;
        std::cout << "[TEST] Model: " << device.model << std::endl;
        std::cout << "[TEST] Version: " << device.version << std::endl;
        std::cout << "[TEST] ========================================" << std::endl;
        
        // Request AllCtrlList property
        std::cout << "[TEST] Requesting AllCtrlList for MUID: 0x" << std::hex << device.muid << std::dec << std::endl;
        
        // First call triggers the property request
        auto ctrlList = controller->getAllCtrlList(device.muid);
        
        if (!ctrlList.has_value()) {
            std::cout << "[TEST] Initial request returned no data (expected - request sent), waiting for GetPropertyDataReply..." << std::endl;
            
            // Wait for GetPropertyDataReply to arrive and be processed
            bool receivedReply = false;
            int maxWaitSeconds = 10;
            int waitedSeconds = 0;
            
            while (!receivedReply && waitedSeconds < maxWaitSeconds) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                waitedSeconds++;
                
                // Check if we received the property data
                ctrlList = controller->getAllCtrlList(device.muid);
                if (ctrlList.has_value()) {
                    receivedReply = true;
                    std::cout << "[TEST] GetPropertyDataReply received after " << waitedSeconds << " seconds" << std::endl;
                }
                
                // Check if we got a property update callback
                if (properties_updated_muids.count(device.muid) > 0) {
                    std::cout << "[TEST] Property update callback triggered for MUID: 0x" << std::hex << device.muid << std::dec << std::endl;
                }
            }
            
            if (!receivedReply) {
                std::cout << "[TEST] No GetPropertyDataReply received after " << maxWaitSeconds << " seconds" << std::endl;
            }
        } else {
            std::cout << "[TEST] Property data already available (cached from previous request)" << std::endl;
        }
        
        if (ctrlList.has_value() && !ctrlList->empty()) {
            foundValidControlList = true;
            std::cout << "[TEST] SUCCESS: Retrieved " << ctrlList->size() << " controls" << std::endl;
            
            // Verify ordering
            bool isOrdered = verifyControlOrdering(*ctrlList);
            if (!isOrdered) {
                allControlListsOrdered = false;
                std::cout << "[TEST] ERROR: Controls are NOT in correct order!" << std::endl;
            } else {
                std::cout << "[TEST] SUCCESS: Controls are in correct order by ctrlIndex" << std::endl;
            }
            
            // Additional checks for reported issues
            int blankTitleCount = 0;
            for (const auto& ctrl : *ctrlList) {
                if (ctrl.title.empty()) {
                    blankTitleCount++;
                }
            }
            
            if (blankTitleCount > 0) {
                std::cout << "[TEST] WARNING: Found " << blankTitleCount 
                          << " controls with blank titles (this was a reported issue)" << std::endl;
            }
            
        } else {
            std::cout << "[TEST] Device returned no AllCtrlList data or empty list" << std::endl;
            std::cout << "[TEST] This could indicate:" << std::endl;
            std::cout << "[TEST] 1. Device doesn't implement ALL_CTRL_LIST property" << std::endl;
            std::cout << "[TEST] 2. Property parsing failed due to malformed JSON" << std::endl;
            std::cout << "[TEST] 3. Chunked response reconstruction issue" << std::endl;
        }
    }
    
    // Final assertions
    if (!foundValidControlList) {
        std::cout << "[TEST] WARNING: No devices returned valid control lists" << std::endl;
        GTEST_SKIP() << "No devices provided AllCtrlList data for ordering verification";
    }
    
    EXPECT_TRUE(allControlListsOrdered) 
        << "One or more devices returned controls in incorrect order based on ctrlIndex";
    
    std::cout << "[TEST] AllCtrlList ordering verification test completed" << std::endl;
}

TEST_F(AllCtrlListOrderingTest, TestRepeatedPropertyRequests) {
    std::cout << "[TEST] Testing repeated property requests for consistency..." << std::endl;
    
    // This test checks if repeated requests return the same ordering
    ASSERT_TRUE(controller->resetMidiConnections());
    
    auto devicePairs = findMatchingDevicePairs();
    if (devicePairs.empty()) {
        GTEST_SKIP() << "No matching device pairs for testing";
    }
    
    const auto& devicePair = devicePairs[0];
    ASSERT_TRUE(controller->selectInputDevice(devicePair.first));
    ASSERT_TRUE(controller->selectOutputDevice(devicePair.second));
    
    controller->sendMidiCIDiscovery();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    auto devices = controller->getMidiCIDeviceDetails();
    if (devices.empty()) {
        GTEST_SKIP() << "No MIDI-CI devices discovered";
    }
    
    // Test with first device
    const auto& device = devices[0];
    std::cout << "[TEST] Testing repeated requests with device: " << device.device_name << std::endl;
    
    // Make multiple requests and compare results
    std::vector<std::vector<midicci::commonproperties::MidiCIControl>> results;
    
    for (int attempt = 1; attempt <= 3; ++attempt) {
        std::cout << "[TEST] Request attempt " << attempt << std::endl;
        
        auto ctrlList = controller->getAllCtrlList(device.muid);
        if (!ctrlList.has_value()) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            ctrlList = controller->getAllCtrlList(device.muid);
        }
        
        if (ctrlList.has_value()) {
            results.push_back(*ctrlList);
            std::cout << "[TEST] Attempt " << attempt << " returned " << ctrlList->size() << " controls" << std::endl;
        } else {
            std::cout << "[TEST] Attempt " << attempt << " returned no data" << std::endl;
        }
        
        // Small delay between requests
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    if (results.size() < 2) {
        GTEST_SKIP() << "Not enough successful requests to compare consistency";
    }
    
    // Compare results for consistency
    const auto& firstResult = results[0];
    bool allConsistent = true;
    
    for (size_t i = 1; i < results.size(); ++i) {
        const auto& currentResult = results[i];
        
        if (firstResult.size() != currentResult.size()) {
            std::cout << "[TEST] ERROR: Result " << i << " has different size (" 
                      << currentResult.size() << ") than first result (" << firstResult.size() << ")" << std::endl;
            allConsistent = false;
            continue;
        }
        
        // Compare control ordering
        for (size_t j = 0; j < firstResult.size(); ++j) {
            if (firstResult[j].ctrlIndex != currentResult[j].ctrlIndex ||
                firstResult[j].title != currentResult[j].title ||
                firstResult[j].ctrlType != currentResult[j].ctrlType) {
                
                std::cout << "[TEST] ERROR: Control at position " << j 
                          << " differs between requests:" << std::endl;
                std::cout << "[TEST]   First: " << firstResult[j].title 
                          << " (ctrlIndex: [" << static_cast<int>(firstResult[j].ctrlIndex[0]) << "])" << std::endl;
                std::cout << "[TEST]   Current: " << currentResult[j].title 
                          << " (ctrlIndex: [" << static_cast<int>(currentResult[j].ctrlIndex[0]) << "])" << std::endl;
                allConsistent = false;
            }
        }
    }
    
    EXPECT_TRUE(allConsistent) 
        << "Repeated AllCtrlList requests returned inconsistent results";
    
    if (allConsistent) {
        std::cout << "[TEST] SUCCESS: All repeated requests returned consistent results" << std::endl;
    }
}