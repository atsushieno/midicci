#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <string>

#include <midicci/midicci.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include <midicci/tooling/CIToolRepository.hpp>

using namespace midicci::tooling;

class CIDeviceModelPropertyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create repository and device manager
        repository = std::make_unique<CIToolRepository>();
        
        // Initialize managers (like the real app does)
        auto midi_manager = repository->get_midi_device_manager();
        auto ci_manager = repository->get_ci_device_manager();
        
        ASSERT_NE(ci_manager, nullptr) << "CIDeviceManager is null";
        
        if (midi_manager) {
            midi_manager->initialize();
        }
        if (ci_manager) {
            ci_manager->initialize();
        }
        
        // Initialize device model
        device_model = ci_manager->get_device_model();
        ASSERT_NE(device_model, nullptr) << "CIDeviceModel is null";
        
        // Reset callback flags
        property_callback_called = false;
        property_callback_count = 0;
    }

    void TearDown() override {
        device_model.reset();
        repository.reset();
    }

    std::unique_ptr<CIToolRepository> repository;
    std::shared_ptr<CIDeviceModel> device_model;
    
    // Test callback tracking
    bool property_callback_called = false;
    int property_callback_count = 0;
};

TEST_F(CIDeviceModelPropertyTest, CreatePropertyAppearsInPropertyList) {
    // Get initial property count using the correct Kotlin pattern
    auto& propertyFacade = device_model->getDevice()->getPropertyHostFacade();
    auto initial_metadata = propertyFacade.getProperties().getMetadataList();
    size_t initial_count = initial_metadata.size();
    
    // Create a new property
    auto property = device_model->create_new_property();
    ASSERT_NE(property, nullptr);
    
    // Verify property appears in property list using the correct pattern
    auto updated_metadata = propertyFacade.getProperties().getMetadataList();
    EXPECT_EQ(updated_metadata.size(), initial_count + 1);
    
    // Verify the new property is in the list
    std::string new_property_id = property->getPropertyId();
    bool found = false;
    for (const auto& metadata : updated_metadata) {
        if (metadata && metadata->getPropertyId() == new_property_id) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "New property ID '" << new_property_id << "' not found in property list";
    
    // Verify property ID format (should be X-NNNN)
    EXPECT_TRUE(new_property_id.starts_with("X-"));
    EXPECT_EQ(new_property_id.length(), 6); // "X-" + 4 digits
}

TEST_F(CIDeviceModelPropertyTest, CreatePropertyTriggersNotification) {
    // Set up callback to track notifications
    device_model->add_properties_updated_callback([this]() {
        property_callback_called = true;
        property_callback_count++;
    });
    
    // Create a new property
    auto property = device_model->create_new_property();
    ASSERT_NE(property, nullptr);
    
    // Verify callback was triggered
    EXPECT_TRUE(property_callback_called) << "Property updated callback was not called";
    EXPECT_EQ(property_callback_count, 1) << "Expected exactly 1 callback, got " << property_callback_count;
}

TEST_F(CIDeviceModelPropertyTest, MultiplePropertiesCreatedCorrectly) {
    // Get initial count using correct pattern
    auto& propertyFacade = device_model->getDevice()->getPropertyHostFacade();
    auto initial_metadata = propertyFacade.getProperties().getMetadataList();
    size_t initial_count = initial_metadata.size();
    
    // Set up callback tracking
    device_model->add_properties_updated_callback([this]() {
        property_callback_count++;
    });
    
    // Create multiple properties
    std::vector<std::string> created_property_ids;
    const int num_properties = 3;
    
    for (int i = 0; i < num_properties; i++) {
        auto property = device_model->create_new_property();
        ASSERT_NE(property, nullptr);
        created_property_ids.push_back(property->getPropertyId());
    }
    
    // Verify all properties appear in the list using correct pattern
    auto final_metadata = propertyFacade.getProperties().getMetadataList();
    EXPECT_EQ(final_metadata.size(), initial_count + num_properties);
    
    // Verify each created property is in the final list
    for (const auto& created_id : created_property_ids) {
        bool found = false;
        for (const auto& metadata : final_metadata) {
            if (metadata && metadata->getPropertyId() == created_id) {
                found = true;
                break;
            }
        }
        EXPECT_TRUE(found) << "Created property '" << created_id << "' not found in final property list";
    }
    
    // Verify callbacks were triggered for each creation
    EXPECT_EQ(property_callback_count, num_properties) 
        << "Expected " << num_properties << " callbacks, got " << property_callback_count;
}

TEST_F(CIDeviceModelPropertyTest, PropertyMetadataAccessible) {
    // Create a new property
    auto property = device_model->create_new_property();
    ASSERT_NE(property, nullptr);
    
    std::string property_id = property->getPropertyId();
    
    // Verify we can retrieve the property metadata
    auto retrieved_metadata = device_model->get_local_property_metadata(property_id);
    ASSERT_NE(retrieved_metadata, nullptr);
    
    // Verify metadata properties
    EXPECT_EQ(retrieved_metadata->getPropertyId(), property_id);
}

TEST_F(CIDeviceModelPropertyTest, RemovePropertyUpdatesListAndNotifications) {
    // Create a property first
    auto property = device_model->create_new_property();
    ASSERT_NE(property, nullptr);
    std::string property_id = property->getPropertyId();
    
    // Verify it's in the list using correct pattern
    auto& propertyFacade = device_model->getDevice()->getPropertyHostFacade();
    auto metadata_with_new = propertyFacade.getProperties().getMetadataList();
    bool found_before = false;
    for (const auto& metadata : metadata_with_new) {
        if (metadata && metadata->getPropertyId() == property_id) {
            found_before = true;
            break;
        }
    }
    EXPECT_TRUE(found_before);
    
    // Set up callback tracking (reset count)
    property_callback_count = 0;
    device_model->add_properties_updated_callback([this]() {
        property_callback_count++;
    });
    
    // Remove the property
    device_model->remove_local_property(property_id);
    
    // Verify it's no longer in the list using correct pattern
    auto metadata_after_remove = propertyFacade.getProperties().getMetadataList();
    bool found_after = false;
    for (const auto& metadata : metadata_after_remove) {
        if (metadata && metadata->getPropertyId() == property_id) {
            found_after = true;
            break;
        }
    }
    EXPECT_FALSE(found_after) << "Property should have been removed from the list";
    
    // Verify callback was triggered for removal
    EXPECT_GT(property_callback_count, 0) << "Property removal should trigger notification callback";
}