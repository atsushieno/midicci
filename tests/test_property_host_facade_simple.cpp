#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>

#include <midicci/midicci.hpp>

class PropertyHostFacadeTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_unique<midicci::MidiCIDeviceConfiguration>();
        device = std::make_shared<midicci::MidiCIDevice>(12345, *config);
        facade = std::make_unique<midicci::PropertyHostFacade>(*device, *config);
    }

    void TearDown() override {
        facade.reset();
        device.reset();
        config.reset();
    }

    std::unique_ptr<midicci::MidiCIDeviceConfiguration> config;
    std::shared_ptr<midicci::MidiCIDevice> device;
    std::unique_ptr<midicci::PropertyHostFacade> facade;
};

TEST_F(PropertyHostFacadeTest, AddMetadataUpdatesPropertyIds) {
    // Get initial property IDs
    auto initial_ids = facade->get_property_ids();
    size_t initial_count = initial_ids.size();
    
    // Create and add a property metadata
    auto metadata = std::make_unique<midicci::commonproperties::CommonRulesPropertyMetadata>("test-property-123");
    std::string property_id = metadata->getPropertyId();
    
    facade->addMetadata(std::move(metadata));
    
    // Verify property appears in the list
    auto updated_ids = facade->get_property_ids();
    EXPECT_EQ(updated_ids.size(), initial_count + 1);
    
    // Verify the specific property is in the list
    bool found = false;
    for (const auto& id : updated_ids) {
        if (id == property_id) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Property ID '" << property_id << "' not found in property list";
}

TEST_F(PropertyHostFacadeTest, MultiplePropertiesAddedCorrectly) {
    // Get initial count
    auto initial_ids = facade->get_property_ids();
    size_t initial_count = initial_ids.size();
    
    // Add multiple properties
    std::vector<std::string> added_property_ids;
    const int num_properties = 3;
    
    for (int i = 0; i < num_properties; i++) {
        std::string prop_id = "test-property-" + std::to_string(i);
        auto metadata = std::make_unique<midicci::commonproperties::CommonRulesPropertyMetadata>(prop_id);
        added_property_ids.push_back(prop_id);
        facade->addMetadata(std::move(metadata));
    }
    
    // Verify all properties are in the list
    auto final_ids = facade->get_property_ids();
    EXPECT_EQ(final_ids.size(), initial_count + num_properties);
    
    // Verify each added property is found
    for (const auto& added_id : added_property_ids) {
        bool found = false;
        for (const auto& final_id : final_ids) {
            if (final_id == added_id) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Added property '" << added_id << "' not found in final property list";
    }
}

TEST_F(PropertyHostFacadeTest, PropertyMetadataRetrievable) {
    // Add a property
    std::string property_id = "test-retrievable-property";
    auto metadata = std::make_unique<midicci::commonproperties::CommonRulesPropertyMetadata>(property_id);
    facade->addMetadata(std::move(metadata));
    
    // Verify we can retrieve the metadata
    auto retrieved = facade->getPropertyMetadata(property_id);
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getPropertyId(), property_id);
}