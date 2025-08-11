#include <gtest/gtest.h>
#include <vector>
#include <memory>
#include "midicci/midicci.hpp"

using namespace midicci;
using namespace midicci::commonproperties;

TEST(PropertyFixTest, MetadataOnlyProperty) {
    // Create a minimal device configuration
    MidiCIDeviceConfiguration config;
    uint32_t muid = 0x12345678;
    
    // Create device
    auto device = std::make_shared<MidiCIDevice>(muid, config);
    // Created MidiCIDevice
    
    auto& propertyFacade = device->get_property_host_facade();
    // Got PropertyHostFacade
    
    // Create property with metadata only (old way)
    auto property1 = std::make_unique<CommonRulesPropertyMetadata>();
    property1->resource = "X-1234";
    property1->canGet = true;
    property1->canSet = "full";
    property1->canSubscribe = true;
    
    propertyFacade.addMetadata(std::move(property1));
    
    auto metadata_list1 = propertyFacade.get_properties().getMetadataList();
    // After metadata only - Property count should be at least 1
    EXPECT_GE(metadata_list1.size(), 1u);
    
    // Should contain the property we just created
    bool foundProperty = false;
    for (const auto& metadata : metadata_list1) {
        if (metadata && metadata->getPropertyId() == "X-1234") {
            foundProperty = true;
            break;
        }
    }
    EXPECT_TRUE(foundProperty);
}

TEST(PropertyFixTest, MetadataWithValueProperty) {
    // Create a minimal device configuration
    MidiCIDeviceConfiguration config;
    uint32_t muid = 0x12345678;
    
    // Create device
    auto device = std::make_shared<MidiCIDevice>(muid, config);
    
    auto& propertyFacade = device->get_property_host_facade();
    
    // Create property with metadata AND value (new way)
    auto property2 = std::make_unique<CommonRulesPropertyMetadata>();
    property2->resource = "X-5678";
    property2->canGet = true;
    property2->canSet = "full";
    property2->canSubscribe = true;
    
    std::string propertyId = property2->resource;
    propertyFacade.addMetadata(std::move(property2));
    
    // Set initial value
    std::string initialValue = "{}";
    std::vector<uint8_t> initialData(initialValue.begin(), initialValue.end());
    propertyFacade.setPropertyValue(propertyId, "", initialData, false);
    
    auto metadata_list2 = propertyFacade.get_properties().getMetadataList();
    // After metadata + value - Property count should be at least 1
    EXPECT_GE(metadata_list2.size(), 1u);
    
    // Should contain the property we just created
    bool foundProperty = false;
    for (const auto& metadata : metadata_list2) {
        if (metadata && metadata->getPropertyId() == "X-5678") {
            foundProperty = true;
            break;
        }
    }
    EXPECT_TRUE(foundProperty);
}