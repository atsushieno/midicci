#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <chrono>
#include <sstream>
#include "midicci/midicci.hpp"

class BlankTitlesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }
    
    void TearDown() override {
        // Test cleanup
    }
};

TEST_F(BlankTitlesTest, TestJSONParsingWithBlankTitles) {
    std::cout << "[TEST] Testing JSON parsing with blank titles..." << std::endl;
    
    // This is a sample of the actual JSON data received from the device (simplified)
    std::string actualDeviceJson = R"([
        {
            "ctrlIndex": [0, 0],
            "ctrlType": "nrpn",
            "default": 2147483647,
            "defaultCCMap": false,
            "description": "",
            "minMax": [0, 4.294967e+09],
            "numSigBits": 32,
            "paramPath": "",
            "recognize": "absolute",
            "title": "",
            "transmit": "absolute"
        },
        {
            "ctrlIndex": [0, 1],
            "ctrlType": "nrpn",
            "default": 2147483647,
            "defaultCCMap": false,
            "description": "",
            "minMax": [0, 4.294967e+09],
            "numSigBits": 32,
            "paramPath": "",
            "recognize": "absolute",
            "title": "",
            "transmit": "absolute"
        },
        {
            "ctrlIndex": [0, 2],
            "ctrlType": "nrpn",
            "default": 2147483647,
            "defaultCCMap": false,
            "description": "",
            "minMax": [0, 4.294967e+09],
            "numSigBits": 32,
            "paramPath": "",
            "recognize": "absolute",
            "title": "",
            "transmit": "absolute"
        }
    ])";
    
    std::vector<uint8_t> data(actualDeviceJson.begin(), actualDeviceJson.end());
    auto controls = midicci::commonproperties::StandardProperties::parseControlList(data);
    
    std::cout << "[TEST] Parsed " << controls.size() << " controls" << std::endl;
    EXPECT_EQ(controls.size(), 3);
    
    // Verify all titles are blank (this is the actual issue)
    for (size_t i = 0; i < controls.size(); ++i) {
        const auto& ctrl = controls[i];
        std::cout << "[TEST] Control " << i << ":" << std::endl;
        std::cout << "[TEST]   Title: '" << ctrl.title << "' (length: " << ctrl.title.length() << ")" << std::endl;
        std::cout << "[TEST]   CtrlType: " << ctrl.ctrlType << std::endl;
        std::cout << "[TEST]   CtrlIndex: [" << static_cast<int>(ctrl.ctrlIndex[0]) << ", " << static_cast<int>(ctrl.ctrlIndex[1]) << "]" << std::endl;
        
        // The issue: all titles are empty
        EXPECT_TRUE(ctrl.title.empty()) << "Title should be empty (this is the bug we're demonstrating)";
    }
    
    // Verify order is correct by ctrlIndex
    for (size_t i = 1; i < controls.size(); ++i) {
        bool isOrderedCorrectly = std::lexicographical_compare(
            controls[i-1].ctrlIndex.begin(), controls[i-1].ctrlIndex.end(),
            controls[i].ctrlIndex.begin(), controls[i].ctrlIndex.end()
        ) || controls[i-1].ctrlIndex == controls[i].ctrlIndex;
        
        EXPECT_TRUE(isOrderedCorrectly) << "Controls should be in order by ctrlIndex";
    }
    
    std::cout << "[TEST] CONFIRMED: Controls are in correct order, but all have blank titles" << std::endl;
}

TEST_F(BlankTitlesTest, TestGeneratedTitlesForBlankControls) {
    std::cout << "[TEST] Testing fallback title generation for controls with blank titles..." << std::endl;
    
    // Sample control with blank title
    std::string jsonWithBlankTitle = R"([
        {
            "ctrlIndex": [1],
            "ctrlType": "cc",
            "default": 64,
            "title": "",
            "description": "Modulation wheel control"
        },
        {
            "ctrlIndex": [7],
            "ctrlType": "cc", 
            "default": 100,
            "title": "",
            "description": ""
        },
        {
            "ctrlIndex": [0, 1],
            "ctrlType": "nrpn",
            "default": 0,
            "title": "",
            "description": ""
        }
    ])";
    
    std::vector<uint8_t> data(jsonWithBlankTitle.begin(), jsonWithBlankTitle.end());
    auto controls = midicci::commonproperties::StandardProperties::parseControlList(data);
    
    std::cout << "[TEST] Testing fallback title generation..." << std::endl;
    
    for (size_t i = 0; i < controls.size(); ++i) {
        const auto& ctrl = controls[i];
        
        // Generate fallback title if original is blank
        std::string displayTitle = ctrl.title;
        if (displayTitle.empty()) {
            if (ctrl.ctrlType == "cc" && !ctrl.ctrlIndex.empty()) {
                displayTitle = "CC " + std::to_string(ctrl.ctrlIndex[0]);
            } else if (ctrl.ctrlType == "nrpn" && ctrl.ctrlIndex.size() >= 2) {
                displayTitle = "NRPN " + std::to_string(ctrl.ctrlIndex[0]) + ":" + std::to_string(ctrl.ctrlIndex[1]);
            } else if (ctrl.ctrlType == "rpn" && ctrl.ctrlIndex.size() >= 2) {
                displayTitle = "RPN " + std::to_string(ctrl.ctrlIndex[0]) + ":" + std::to_string(ctrl.ctrlIndex[1]);
            } else {
                displayTitle = ctrl.ctrlType + " Control";
            }
        }
        
        std::cout << "[TEST] Control " << i << ": Original='" << ctrl.title << "' -> Generated='" << displayTitle << "'" << std::endl;
        
        // Verify we generated a meaningful title
        EXPECT_FALSE(displayTitle.empty()) << "Generated title should not be empty";
    }
}

TEST_F(BlankTitlesTest, TestPerformanceWithLargeControlList) {
    std::cout << "[TEST] Testing performance with large control list (simulating real device)..." << std::endl;
    
    // Generate a large JSON array similar to what the device sends
    std::ostringstream jsonBuilder;
    jsonBuilder << "[";
    
    const int NUM_CONTROLS = 128; // Simulate a device with many controls
    for (int i = 0; i < NUM_CONTROLS; ++i) {
        if (i > 0) jsonBuilder << ",";
        jsonBuilder << R"({
            "ctrlIndex": [0, )" << i << R"(],
            "ctrlType": "nrpn",
            "default": 0,
            "defaultCCMap": false,
            "description": "",
            "minMax": [0, 4294967295],
            "numSigBits": 32,
            "paramPath": "",
            "recognize": "absolute",
            "title": "",
            "transmit": "absolute"
        })";
    }
    jsonBuilder << "]";
    
    std::string largeJson = jsonBuilder.str();
    std::cout << "[TEST] Generated JSON size: " << largeJson.length() << " bytes" << std::endl;
    
    // Measure parsing time
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<uint8_t> data(largeJson.begin(), largeJson.end());
    auto controls = midicci::commonproperties::StandardProperties::parseControlList(data);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "[TEST] Parsed " << controls.size() << " controls in " << duration.count() << "ms" << std::endl;
    
    EXPECT_EQ(controls.size(), NUM_CONTROLS);
    EXPECT_LT(duration.count(), 100) << "Parsing should be fast (< 100ms)";
    
    // Verify all controls have blank titles
    int blankTitleCount = 0;
    for (const auto& ctrl : controls) {
        if (ctrl.title.empty()) {
            blankTitleCount++;
        }
    }
    
    std::cout << "[TEST] Controls with blank titles: " << blankTitleCount << "/" << controls.size() << std::endl;
    EXPECT_EQ(blankTitleCount, NUM_CONTROLS) << "All controls should have blank titles in this test";
}