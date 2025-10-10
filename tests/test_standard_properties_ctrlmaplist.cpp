#include <gtest/gtest.h>
#include "midicci/midicci.hpp"

using namespace midicci;
using namespace midicci::commonproperties;

class StandardPropertiesCtrlMapListTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_unique<MidiCIDeviceConfiguration>();
        device = std::make_unique<MidiCIDevice>(0x12345678, *config);
    }

    std::unique_ptr<MidiCIDeviceConfiguration> config;
    std::unique_ptr<MidiCIDevice> device;
};

TEST_F(StandardPropertiesCtrlMapListTest, MidiCIControlMapConstruction) {
    MidiCIControlMap map(42, "Test Control Map");
    EXPECT_EQ(map.value, 42);
    EXPECT_EQ(map.title, "Test Control Map");
}

TEST_F(StandardPropertiesCtrlMapListTest, ParseAndSerializeControlMapList) {
    // Create test data
    std::vector<MidiCIControlMap> original = {
        MidiCIControlMap(0, "Off"),
        MidiCIControlMap(127, "Max"),
        MidiCIControlMap(64, "Center")
    };
    
    // Convert to JSON
    std::vector<uint8_t> json_data = StandardProperties::toJson(original);
    EXPECT_FALSE(json_data.empty());
    
    // Parse back from JSON
    std::vector<MidiCIControlMap> parsed = StandardProperties::parseControlMapList(json_data);
    
    EXPECT_EQ(parsed.size(), 3);
    EXPECT_EQ(parsed[0].value, 0);
    EXPECT_EQ(parsed[0].title, "Off");
    EXPECT_EQ(parsed[1].value, 127);
    EXPECT_EQ(parsed[1].title, "Max");
    EXPECT_EQ(parsed[2].value, 64);
    EXPECT_EQ(parsed[2].title, "Center");
}

TEST_F(StandardPropertiesCtrlMapListTest, DeviceExtensionFunctions) {
    std::vector<MidiCIControlMap> maps = {
        MidiCIControlMap(0, "Off"),
        MidiCIControlMap(127, "Max")
    };
    
    // Set control map list
    StandardPropertiesExtensions::setCtrlMapList(*device, "testControl", maps);
    
    // Get control map list
    auto result = StandardPropertiesExtensions::getCtrlMapList(*device, "testControl");
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->size(), 2);
    EXPECT_EQ((*result)[0].value, 0);
    EXPECT_EQ((*result)[0].title, "Off");
    EXPECT_EQ((*result)[1].value, 127);
    EXPECT_EQ((*result)[1].title, "Max");
}

TEST_F(StandardPropertiesCtrlMapListTest, CtrlMapListMetadata) {
    // Test metadata
    EXPECT_TRUE(StandardProperties::ctrlMapListMetadata().requireResId);
    EXPECT_EQ(StandardProperties::ctrlMapListMetadata().columns.size(), 2);
    EXPECT_EQ(StandardProperties::ctrlMapListMetadata().columns[0].property, "value");
    EXPECT_EQ(StandardProperties::ctrlMapListMetadata().columns[1].property, "title");
}

TEST_F(StandardPropertiesCtrlMapListTest, ParseEmptyControlMapList) {
    std::vector<uint8_t> empty_json = {'[', ']'};
    std::vector<MidiCIControlMap> parsed = StandardProperties::parseControlMapList(empty_json);
    EXPECT_TRUE(parsed.empty());
}

TEST_F(StandardPropertiesCtrlMapListTest, GetNonExistentControlMap) {
    auto result = StandardPropertiesExtensions::getCtrlMapList(*device, "nonExistent");
    EXPECT_FALSE(result.has_value());
}