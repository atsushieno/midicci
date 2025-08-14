#include <gtest/gtest.h>
#include <memory>
#include <iostream>
#include <vector>
#include <string>

// Include midicci headers directly to test StandardProperties
#include "midicci/midicci.hpp"

using namespace midicci::commonproperties;

class PropertiesParsingTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(PropertiesParsingTest, TestParseControlListWithValidJSON) {
    // Test with the expected format - array of control objects
    std::string validJson = R"([
        {
            "title": "Modulation",
            "ctrlType": "cc",
            "description": "Modulation wheel",
            "ctrlIndex": [1],
            "channel": 1,
            "priority": 1,
            "default": 0
        },
        {
            "title": "Volume", 
            "ctrlType": "cc",
            "description": "Channel volume",
            "ctrlIndex": [7],
            "channel": 1,
            "priority": 1,
            "default": 127
        },
        {
            "title": "Pan",
            "ctrlType": "cc", 
            "description": "Stereo pan",
            "ctrlIndex": [10],
            "channel": 1,
            "priority": 2,
            "default": 64
        }
    ])";
    
    std::vector<uint8_t> data(validJson.begin(), validJson.end());
    
    std::cout << "[TEST] Testing parseControlList with JSON: " << validJson << std::endl;
    
    auto result = StandardProperties::parseControlList(data);
    
    std::cout << "[TEST] ParseControlList returned " << result.size() << " items" << std::endl;
    
    EXPECT_EQ(result.size(), 3) << "Expected 3 control items";
    
    if (!result.empty()) {
        const auto& first = result[0];
        std::cout << "[TEST] First item:" << std::endl;
        std::cout << "[TEST]   Title: " << first.title << std::endl;
        std::cout << "[TEST]   CtrlType: " << first.ctrlType << std::endl;
        std::cout << "[TEST]   Description: " << first.description << std::endl;
        
        EXPECT_EQ(first.title, "Modulation");
        EXPECT_EQ(first.ctrlType, "cc");
        EXPECT_EQ(first.description, "Modulation wheel");
    }
}

TEST_F(PropertiesParsingTest, TestParseControlListWithWrappedJSON) {
    // Test with the format that might be coming from actual devices - wrapped in ctrlList
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
    
    std::vector<uint8_t> data(wrappedJson.begin(), wrappedJson.end());
    
    std::cout << "[TEST] Testing parseControlList with wrapped JSON: " << wrappedJson << std::endl;
    
    auto result = StandardProperties::parseControlList(data);
    
    std::cout << "[TEST] ParseControlList returned " << result.size() << " items" << std::endl;
    
    // The parser should now handle both wrapped and direct formats
    if (result.size() == 2) {
        std::cout << "[TEST] SUCCESS: Parser now handles wrapped 'ctrlList' format correctly!" << std::endl;
        const auto& first = result[0];
        std::cout << "[TEST] First item from wrapped format:" << std::endl;
        std::cout << "[TEST]   Title: " << first.title << std::endl;
        std::cout << "[TEST]   CtrlType: " << first.ctrlType << std::endl;
    } else {
        std::cout << "[TEST] UNEXPECTED: Expected 2 items from wrapped format" << std::endl;
    }
    
    EXPECT_EQ(result.size(), 2) << "Wrapped format should now work with fixed parser implementation";
}

TEST_F(PropertiesParsingTest, TestParseControlListWithEmptyData) {
    std::vector<uint8_t> emptyData;
    
    auto result = StandardProperties::parseControlList(emptyData);
    
    std::cout << "[TEST] Empty data returned " << result.size() << " items" << std::endl;
    EXPECT_EQ(result.size(), 0);
}

TEST_F(PropertiesParsingTest, TestParseControlListWithInvalidJSON) {
    std::string invalidJson = R"({ "invalid": "json" without proper array })";
    std::vector<uint8_t> data(invalidJson.begin(), invalidJson.end());
    
    auto result = StandardProperties::parseControlList(data);
    
    std::cout << "[TEST] Invalid JSON returned " << result.size() << " items" << std::endl;
    EXPECT_EQ(result.size(), 0);
}