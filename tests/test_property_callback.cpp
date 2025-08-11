#include <gtest/gtest.h>
#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>

using namespace midicci::tooling;

TEST(PropertyCallbackTest, CallbackFlow) {
    // Create repository
    auto repository = std::make_unique<CIToolRepository>();
    // Created repository
    
    ASSERT_NE(repository->get_ci_device_manager(), nullptr);

    repository->get_ci_device_manager()->initialize();
    auto deviceModel = repository->get_ci_device_manager()->get_device_model();
    ASSERT_NE(deviceModel, nullptr);
    
    // Got device model
    
    // Register callback
    bool callbackCalled = false;
    deviceModel->add_properties_updated_callback([&callbackCalled]() {
        // Properties updated callback called!
        callbackCalled = true;
    });
    
    // Callback registered
    
    // Create property
    auto property = deviceModel->create_new_property();
    ASSERT_NE(property, nullptr);
    
    // Property created with ID
    EXPECT_FALSE(property->getPropertyId().empty());
    
    // Check if callback was called
    EXPECT_TRUE(callbackCalled);
    
    // Check property list using the correct pattern
    auto& propertyFacade = deviceModel->get_device()->get_property_host_facade();
    auto metadata_list = propertyFacade.get_properties().getMetadataList();
    // Total properties should be at least 1
    EXPECT_GE(metadata_list.size(), 1u);
    
    // Should contain the property we just created
    bool foundProperty = false;
    std::string created_id = property->getPropertyId();
    for (const auto& metadata : metadata_list) {
        if (metadata && metadata->getPropertyId() == created_id) {
            foundProperty = true;
            break;
        }
    }
    EXPECT_TRUE(foundProperty);
}