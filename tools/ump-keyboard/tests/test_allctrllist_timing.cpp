#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include "keyboard_controller.h"

class AllCtrlListTimingTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_unique<KeyboardController>();
        
        // Track property update callbacks with detailed logging
        controller->setMidiCIPropertiesChangedCallback([this](uint32_t muid, const std::string&, const std::string&) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - test_start_time);
            
            std::cout << "[CALLBACK] Properties updated for MUID: 0x" << std::hex << muid << std::dec 
                      << " at " << elapsed.count() << "ms" << std::endl;
            
            property_update_count++;
            last_updated_muid = muid;
            
            // Check what properties are available now
            auto ctrlList = controller->getAllCtrlList(muid);
            if (ctrlList.has_value()) {
                std::cout << "[CALLBACK] AllCtrlList now available with " << ctrlList->size() << " controls" << std::endl;
                control_list_available = true;
            } else {
                std::cout << "[CALLBACK] AllCtrlList still not available" << std::endl;
            }
        });
    }
    
    void TearDown() override {
        if (controller) {
            controller.reset();
        }
    }
    
    std::unique_ptr<KeyboardController> controller;
    std::chrono::steady_clock::time_point test_start_time;
    std::atomic<int> property_update_count{0};
    std::atomic<uint32_t> last_updated_muid{0};
    std::atomic<bool> control_list_available{false};
    
    std::vector<std::pair<std::string, std::string>> findMatchingDevicePairs() {
        std::vector<std::pair<std::string, std::string>> pairs;
        
        auto inputDevices = controller->getInputDevices();
        auto outputDevices = controller->getOutputDevices();
        
        for (const auto& input : inputDevices) {
            for (const auto& output : outputDevices) {
                if (input.second == output.second) {
                    // Skip MIDI Through ports on Linux as they're loopback ports without MIDI-CI
                    if (input.second.find("MIDI Through") != std::string::npos ||
                        input.second.find("Midi Through") != std::string::npos) {
                        std::cout << "[TEST] Skipping MIDI Through port: " << input.second << std::endl;
                        continue;
                    }
                    
                    pairs.emplace_back(input.first, output.first);
                    std::cout << "[TEST] Found matching pair: " << input.second << std::endl;
                }
            }
        }
        
        return pairs;
    }
};

TEST_F(AllCtrlListTimingTest, TestAsyncPropertyRequestTiming) {
    std::cout << "[TEST] Testing asynchronous property request timing..." << std::endl;
    test_start_time = std::chrono::steady_clock::now();
    
    // Initialize MIDI connections
    ASSERT_TRUE(controller->resetMidiConnections());
    
    // Find matching device pairs
    auto devicePairs = findMatchingDevicePairs();
    if (devicePairs.empty()) {
        GTEST_SKIP() << "No matching MIDI device pairs available for testing";
    }
    
    // Select device pair
    const auto& devicePair = devicePairs[0];
    ASSERT_TRUE(controller->selectInputDevice(devicePair.first));
    ASSERT_TRUE(controller->selectOutputDevice(devicePair.second));
    
    // Send discovery and wait for devices
    std::cout << "[TEST] Sending MIDI-CI discovery..." << std::endl;
    controller->sendMidiCIDiscovery();
    
    // Wait for discovery with progress reporting
    std::cout << "[TEST] Waiting for device discovery..." << std::endl;
    std::vector<MidiCIDeviceInfo> devices;
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        devices = controller->getMidiCIDeviceDetails();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - test_start_time);
        std::cout << "[TEST] After " << elapsed.count() << "ms: " << devices.size() << " devices discovered" << std::endl;
        
        if (!devices.empty()) {
            std::cout << "[TEST] Discovery complete!" << std::endl;
            break;
        }
    }
    
    if (devices.empty()) {
        GTEST_SKIP() << "No MIDI-CI devices discovered";
    }
    
    const auto& device = devices[0];
    std::cout << "[TEST] Testing with device MUID: 0x" << std::hex << device.muid << std::dec << std::endl;
    
    // Reset tracking variables
    property_update_count = 0;
    control_list_available = false;
    
    // Make the property request and time it
    auto request_start = std::chrono::steady_clock::now();
    std::cout << "[TEST] Making getAllCtrlList request..." << std::endl;
    
    auto ctrlList = controller->getAllCtrlList(device.muid);
    
    auto request_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - request_start);
    std::cout << "[TEST] Initial request returned after " << request_elapsed.count() << "ms" << std::endl;
    
    if (ctrlList.has_value()) {
        std::cout << "[TEST] Immediate data available: " << ctrlList->size() << " controls" << std::endl;
        EXPECT_GT(ctrlList->size(), 0) << "Should have controls if data is immediately available";
    } else {
        std::cout << "[TEST] No immediate data - waiting for async response..." << std::endl;
        
        // Wait for property update with detailed timing
        bool receivedData = false;
        const int maxWaitSeconds = 15;
        
        for (int second = 1; second <= maxWaitSeconds; ++second) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            auto total_elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - test_start_time);
            
            std::cout << "[TEST] After " << second << "s (total " << total_elapsed.count() 
                      << "ms): callbacks=" << property_update_count.load() 
                      << ", available=" << control_list_available.load() << std::endl;
            
            // Check for data
            ctrlList = controller->getAllCtrlList(device.muid);
            if (ctrlList.has_value()) {
                receivedData = true;
                std::cout << "[TEST] Data received after " << second << " seconds!" << std::endl;
                std::cout << "[TEST] Control list size: " << ctrlList->size() << std::endl;
                break;
            }
        }
        
        if (!receivedData) {
            std::cout << "[TEST] ERROR: No data received after " << maxWaitSeconds << " seconds" << std::endl;
            std::cout << "[TEST] Total property callbacks: " << property_update_count.load() << std::endl;
            GTEST_FAIL() << "Expected to receive AllCtrlList data within " << maxWaitSeconds << " seconds";
        }
    }
    
    // If we got data, verify it's correct
    if (ctrlList.has_value() && !ctrlList->empty()) {
        std::cout << "[TEST] SUCCESS: Received " << ctrlList->size() << " controls" << std::endl;
        
        // Test a few controls for ordering
        if (ctrlList->size() >= 2) {
            const auto& first = (*ctrlList)[0];
            const auto& second = (*ctrlList)[1];
            
            std::cout << "[TEST] First control - ctrlIndex: [";
            for (size_t i = 0; i < first.ctrlIndex.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << static_cast<int>(first.ctrlIndex[i]);
            }
            std::cout << "], title: '" << first.title << "'" << std::endl;
            
            std::cout << "[TEST] Second control - ctrlIndex: [";
            for (size_t i = 0; i < second.ctrlIndex.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << static_cast<int>(second.ctrlIndex[i]);
            }
            std::cout << "], title: '" << second.title << "'" << std::endl;
            
            // Verify ordering
            bool isOrdered = std::lexicographical_compare(
                first.ctrlIndex.begin(), first.ctrlIndex.end(),
                second.ctrlIndex.begin(), second.ctrlIndex.end()
            ) || first.ctrlIndex == second.ctrlIndex;
            
            EXPECT_TRUE(isOrdered) << "Controls should be ordered by ctrlIndex";
            
            // Check for blank titles (the reported issue)
            if (first.title.empty() && second.title.empty()) {
                std::cout << "[TEST] CONFIRMED: Blank title issue exists" << std::endl;
            }
        }
        
        // Verify all controls are properly ordered
        bool allOrdered = true;
        for (size_t i = 1; i < ctrlList->size(); ++i) {
            bool thisOrdered = std::lexicographical_compare(
                (*ctrlList)[i-1].ctrlIndex.begin(), (*ctrlList)[i-1].ctrlIndex.end(),
                (*ctrlList)[i].ctrlIndex.begin(), (*ctrlList)[i].ctrlIndex.end()
            ) || (*ctrlList)[i-1].ctrlIndex == (*ctrlList)[i].ctrlIndex;
            
            if (!thisOrdered) {
                allOrdered = false;
                std::cout << "[TEST] ERROR: Controls " << (i-1) << " and " << i << " are out of order" << std::endl;
            }
        }
        
        EXPECT_TRUE(allOrdered) << "All controls should be properly ordered";
        
        if (allOrdered) {
            std::cout << "[TEST] SUCCESS: All " << ctrlList->size() << " controls are properly ordered" << std::endl;
        }
    }
}