#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <thread>
#include <chrono>
#include <algorithm>
#include "keyboard_controller.h"

class OrderingBugAnalysisTest : public ::testing::Test {
protected:
    void SetUp() override {
        controller = std::make_unique<KeyboardController>();
        
        controller->setMidiCIPropertiesChangedCallback([this](uint32_t muid, const std::string&) {
            auto ctrlList = controller->getAllCtrlList(muid);
            if (ctrlList.has_value()) {
                data_received = true;
                data_received_muid = muid;
            }
        });
    }
    
    void TearDown() override {
        if (controller) {
            controller.reset();
        }
    }
    
    std::unique_ptr<KeyboardController> controller;
    std::atomic<bool> data_received{false};
    std::atomic<uint32_t> data_received_muid{0};
    
    std::vector<std::pair<std::string, std::string>> findMatchingDevicePairs() {
        std::vector<std::pair<std::string, std::string>> pairs;
        
        auto inputDevices = controller->getInputDevices();
        auto outputDevices = controller->getOutputDevices();
        
        for (const auto& input : inputDevices) {
            for (const auto& output : outputDevices) {
                if (input.second == output.second) {
                    pairs.emplace_back(input.first, output.first);
                }
            }
        }
        
        return pairs;
    }
    
    void analyzeOrderingIssues(const std::vector<midicci::commonproperties::MidiCIControl>& controls) {
        std::cout << "[ANALYSIS] Analyzing ordering issues in " << controls.size() << " controls..." << std::endl;
        
        struct ControlInfo {
            size_t index;
            std::vector<uint8_t> ctrlIndex;
            std::string title;
            std::string ctrlType;
        };
        
        std::vector<ControlInfo> misordered_controls;
        
        for (size_t i = 1; i < controls.size(); ++i) {
            const auto& prev = controls[i-1];
            const auto& curr = controls[i];
            
            // Check if current control's ctrlIndex should come before previous
            bool isOrdered = std::lexicographical_compare(
                prev.ctrlIndex.begin(), prev.ctrlIndex.end(),
                curr.ctrlIndex.begin(), curr.ctrlIndex.end()
            ) || prev.ctrlIndex == curr.ctrlIndex;
            
            if (!isOrdered) {
                misordered_controls.push_back({
                    .index = i,
                    .ctrlIndex = curr.ctrlIndex,
                    .title = curr.title,
                    .ctrlType = curr.ctrlType
                });
                
                std::cout << "[ORDERING BUG] Position " << i << ":" << std::endl;
                std::cout << "  Previous [" << (i-1) << "]: ctrlIndex=[";
                for (size_t j = 0; j < prev.ctrlIndex.size(); ++j) {
                    if (j > 0) std::cout << ",";
                    std::cout << static_cast<int>(prev.ctrlIndex[j]);
                }
                std::cout << "], title='" << prev.title << "', type=" << prev.ctrlType << std::endl;
                
                std::cout << "  Current  [" << i << "]: ctrlIndex=[";
                for (size_t j = 0; j < curr.ctrlIndex.size(); ++j) {
                    if (j > 0) std::cout << ",";
                    std::cout << static_cast<int>(curr.ctrlIndex[j]);
                }
                std::cout << "], title='" << curr.title << "', type=" << curr.ctrlType << std::endl;
                std::cout << std::endl;
            }
        }
        
        std::cout << "[ANALYSIS] Found " << misordered_controls.size() << " ordering violations" << std::endl;
        
        if (!misordered_controls.empty()) {
            // Analyze patterns in the misordered controls
            std::cout << "[PATTERN ANALYSIS] Examining patterns in misordered controls:" << std::endl;
            
            // Group by ctrlIndex patterns
            std::map<std::pair<uint8_t, uint8_t>, std::vector<size_t>> patterns;
            
            for (const auto& ctrl : misordered_controls) {
                if (ctrl.ctrlIndex.size() >= 2) {
                    auto key = std::make_pair(ctrl.ctrlIndex[0], ctrl.ctrlIndex[1]);
                    patterns[key].push_back(ctrl.index);
                }
            }
            
            for (const auto& [pattern, indices] : patterns) {
                std::cout << "  Pattern [" << static_cast<int>(pattern.first) 
                          << "," << static_cast<int>(pattern.second) << "] appears at positions: ";
                for (size_t i = 0; i < indices.size(); ++i) {
                    if (i > 0) std::cout << ", ";
                    std::cout << indices[i];
                }
                std::cout << std::endl;
            }
            
            // Check if certain ctrlIndex ranges appear out of order
            std::cout << "[RANGE ANALYSIS] Examining ctrlIndex ranges:" << std::endl;
            
            std::map<uint8_t, std::pair<size_t, size_t>> first_byte_ranges; // first_byte -> (min_pos, max_pos)
            
            for (size_t i = 0; i < controls.size(); ++i) {
                if (!controls[i].ctrlIndex.empty()) {
                    uint8_t first_byte = controls[i].ctrlIndex[0];
                    
                    if (first_byte_ranges.find(first_byte) == first_byte_ranges.end()) {
                        first_byte_ranges[first_byte] = {i, i};
                    } else {
                        first_byte_ranges[first_byte].first = std::min(first_byte_ranges[first_byte].first, i);
                        first_byte_ranges[first_byte].second = std::max(first_byte_ranges[first_byte].second, i);
                    }
                }
            }
            
            for (const auto& [byte_val, range] : first_byte_ranges) {
                if (range.second - range.first > 100) { // Only show ranges that span many positions
                    std::cout << "  First byte " << static_cast<int>(byte_val) 
                              << " appears from position " << range.first 
                              << " to " << range.second 
                              << " (span: " << (range.second - range.first + 1) << ")" << std::endl;
                }
            }
        }
    }
};

TEST_F(OrderingBugAnalysisTest, AnalyzeRealDeviceOrderingBug) {
    std::cout << "[TEST] Analyzing ordering bug in real device data..." << std::endl;
    
    ASSERT_TRUE(controller->resetMidiConnections());
    
    auto devicePairs = findMatchingDevicePairs();
    if (devicePairs.empty()) {
        GTEST_SKIP() << "No matching device pairs available";
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
    
    const auto& device = devices[0];
    std::cout << "[TEST] Analyzing device: " << device.device_name << std::endl;
    
    data_received = false;
    auto ctrlList = controller->getAllCtrlList(device.muid);
    
    if (!ctrlList.has_value()) {
        // Wait for async response
        for (int i = 0; i < 15; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (data_received.load()) {
                ctrlList = controller->getAllCtrlList(device.muid);
                if (ctrlList.has_value()) {
                    break;
                }
            }
        }
    }
    
    ASSERT_TRUE(ctrlList.has_value()) << "Failed to get AllCtrlList data";
    ASSERT_FALSE(ctrlList->empty()) << "AllCtrlList is empty";
    
    std::cout << "[TEST] Retrieved " << ctrlList->size() << " controls for analysis" << std::endl;
    
    // Perform detailed ordering analysis
    analyzeOrderingIssues(*ctrlList);
    
    // Don't fail the test - we're just analyzing the data
    std::cout << "[TEST] Ordering analysis completed" << std::endl;
}