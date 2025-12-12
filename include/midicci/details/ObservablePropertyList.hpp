#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>
#include <mutex>
#include <optional>
#include "midicci/midicci.hpp"

namespace midicci {

class SubscribeProperty;

namespace commonproperties {
class MidiCIServicePropertyRules;
}

using namespace midicci::commonproperties;
class MidiCIClientPropertyRules;


class ObservablePropertyList {
public:
    virtual ~ObservablePropertyList() = default;
    
    virtual std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const = 0;
    virtual std::vector<PropertyValue> getValues() const = 0;
    
    // Pure virtual method for setting property values - to be implemented by derived classes
    virtual void setPropertyValue(const std::string& propertyId, const std::string& resId, 
                                  const std::vector<uint8_t>& data, bool isPartial = false) = 0;
    
    void addPropertyUpdatedCallback(PropertyUpdatedCallback callback);
    void addPropertyCatalogUpdatedCallback(PropertyCatalogUpdatedCallback callback);
    
    void removePropertyUpdatedCallback(const PropertyUpdatedCallback& callback);
    void removePropertyCatalogUpdatedCallback(const PropertyCatalogUpdatedCallback& callback);

protected:
    void notifyPropertyUpdated(const std::string& propertyId, const std::string& resId);
    void notifyPropertyCatalogUpdated();
    
private:
    std::vector<PropertyUpdatedCallback> property_updated_callbacks_;
    std::vector<PropertyCatalogUpdatedCallback> property_catalog_updated_callbacks_;
    mutable std::recursive_mutex mutex_;
};

class ClientObservablePropertyList : public ObservablePropertyList {
public:
    explicit ClientObservablePropertyList(MidiCIClientPropertyRules* property_client);
    ~ClientObservablePropertyList() override = default;
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const override;
    std::vector<PropertyValue> getValues() const override;
    
    void setPropertyValue(const std::string& propertyId, const std::string& resId, 
                          const std::vector<uint8_t>& data, bool isPartial = false) override;

    void updateValue(const std::string& propertyId, const std::vector<uint8_t>& body, const std::string& mediaType = "application/json");
    std::string updateValue(const SubscribeProperty& msg);
    
private:
    MidiCIClientPropertyRules* property_client_;
    std::map<std::string, PropertyValue> values_;
    mutable std::recursive_mutex mutex_;
};

class ServiceObservablePropertyList : public ObservablePropertyList {
public:
    ServiceObservablePropertyList(std::vector<PropertyValue>& internalValues, MidiCIServicePropertyRules& propertyService);
    ~ServiceObservablePropertyList() override = default;
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const override;
    std::vector<PropertyValue> getValues() const override;
    
    void setPropertyValue(const std::string& propertyId, const std::string& resId, 
                          const std::vector<uint8_t>& data, bool isPartial = false) override;

    // Safer method to get metadata by property ID without ownership transfer
    const PropertyMetadata* getMetadata(const std::string& property_id) const;
    
    // Direct access to internal values for PropertyHostFacade (following Kotlin pattern)
    std::vector<PropertyValue>& getMutableValues();
    
    void addMetadata(std::unique_ptr<PropertyMetadata> metadata);
    void updateMetadata(const std::string& propertyId, PropertyMetadata* metadata);
    void updateValue(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    void updateValue(const std::string& propertyId, const std::string& resId, const std::string& mediaType, const std::vector<uint8_t>& body);
    void removeMetadata(const std::string& propertyId);
    
private:
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<PropertyValue>& internal_values_;
    midicci::commonproperties::MidiCIServicePropertyRules& property_service_;
    mutable std::recursive_mutex mutex_;
};

} // namespace
