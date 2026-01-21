#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>

#include <midicci/midicci.hpp>

class IntegrationPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        config = std::make_unique<midicci::MidiCIDeviceConfiguration>();
        device = std::make_shared<midicci::MidiCIDevice>(12345, *config);
        property_facade = &device->getPropertyHostFacade();
        
        // Track callbacks
        callback_count = 0;
    }

    void TearDown() override {
        device.reset();
        config.reset();
    }

    std::unique_ptr<midicci::MidiCIDeviceConfiguration> config;
    std::shared_ptr<midicci::MidiCIDevice> device;
    midicci::PropertyHostFacade* property_facade;
    int callback_count = 0;
};

TEST_F(IntegrationPropertyTest, CreatePropertyAndCheckList) {
    // Get initial property list
    auto initial_ids = property_facade->get_property_ids();
    size_t initial_count = initial_ids.size();
    
    // Create and add a property (this simulates what CIDeviceModel::create_new_property does)
    auto metadata = std::make_unique<midicci::commonproperties::CommonRulesPropertyMetadata>();
    metadata->resource = "X-1234";  // Simulate the generated ID
    metadata->canGet = true;
    metadata->canSet = "full";
    metadata->canSubscribe = true;
    metadata->requireResId = false;
    metadata->mediaTypes = {"application/json"};
    metadata->encodings = {"ASCII"};
    metadata->schema = "{}";
    metadata->canPaginate = false;
    
    std::string property_id = metadata->resource;
    property_facade->addMetadata(std::move(metadata));
    
    // Check if property appears in the list
    auto updated_ids = property_facade->get_property_ids();
    EXPECT_EQ(updated_ids.size(), initial_count + 1) << "Property count should increase by 1";
    
    // Verify the specific property is in the list
    bool found = false;
    for (const auto& id : updated_ids) {
        if (id == property_id) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Property ID '" << property_id << "' should be found in property list";
    
    // Verify we can retrieve the metadata
    auto retrieved = property_facade->getPropertyMetadata(property_id);
    ASSERT_NE(retrieved, nullptr) << "Should be able to retrieve property metadata";
    EXPECT_EQ(retrieved->getPropertyId(), property_id) << "Retrieved property should have correct ID";
}

TEST_F(IntegrationPropertyTest, PropertyListUpdateAfterMultipleCreations) {
    // Get initial count
    auto initial_ids = property_facade->get_property_ids();
    size_t initial_count = initial_ids.size();
    
    // Create multiple properties
    std::vector<std::string> created_ids;
    for (int i = 0; i < 3; i++) {
        auto metadata = std::make_unique<midicci::commonproperties::CommonRulesPropertyMetadata>();
        metadata->resource = "X-" + std::to_string(1000 + i);
        created_ids.push_back(metadata->resource);
        property_facade->addMetadata(std::move(metadata));
    }
    
    // Verify all properties are in the final list
    auto final_ids = property_facade->get_property_ids();
    EXPECT_EQ(final_ids.size(), initial_count + 3) << "Should have 3 more properties";
    
    // Verify each created property is found
    for (const auto& created_id : created_ids) {
        bool found = false;
        for (const auto& final_id : final_ids) {
            if (final_id == created_id) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Created property '" << created_id << "' not found in final list";
    }
}