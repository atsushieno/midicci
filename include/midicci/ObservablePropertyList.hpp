#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <string>
#include <map>
#include <mutex>

namespace midicci {

class SubscribeProperty;

namespace commonproperties {

class MidiCIServicePropertyRules;

class PropertyMetadata {
public:
    virtual ~PropertyMetadata() = default;
    
    virtual const std::string& getPropertyId() const = 0;
    virtual const std::string& getResourceId() const = 0;
    virtual const std::string& getName() const = 0;
    virtual const std::string& getMediaType() const = 0;
    virtual const std::string& getEncoding() const = 0;
    virtual const std::vector<uint8_t>& getData() const = 0;
    virtual std::string getExtra(const std::string& key) const = 0;
};
}

    using namespace midicci::commonproperties;

class MidiCIClientPropertyRules;

struct PropertyValue {
    std::string id;
    std::string resId;  // Resource ID (can be empty for properties without resource ID)
    std::string mediaType;
    std::vector<uint8_t> body;
    
    PropertyValue(const std::string& property_id, const std::string& media_type, const std::vector<uint8_t>& data)
        : id(property_id), resId(""), mediaType(media_type), body(data) {}
    
    PropertyValue(const std::string& property_id, const std::string& resource_id, const std::string& media_type, const std::vector<uint8_t>& data)
        : id(property_id), resId(resource_id), mediaType(media_type), body(data) {}
    
    bool operator==(const PropertyValue& other) const {
        return id == other.id && resId == other.resId && mediaType == other.mediaType && body == other.body;
    }
};

struct SubscriptionEntry {
    uint32_t muid;
    std::string resource;
    std::string subscribe_id;
    std::string encoding;
    
    SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                     const std::string& sub_id, const std::string& enc);
};

using PropertyUpdatedCallback = std::function<void(const std::string&)>;
using PropertyCatalogUpdatedCallback = std::function<void()>;

class ObservablePropertyList {
public:
    virtual ~ObservablePropertyList() = default;
    
    virtual std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const = 0;
    virtual std::vector<PropertyValue> getValues() const = 0;
    
    void addPropertyUpdatedCallback(PropertyUpdatedCallback callback);
    void addPropertyCatalogUpdatedCallback(PropertyCatalogUpdatedCallback callback);
    
    void removePropertyUpdatedCallback(const PropertyUpdatedCallback& callback);
    void removePropertyCatalogUpdatedCallback(const PropertyCatalogUpdatedCallback& callback);

protected:
    void notifyPropertyUpdated(const std::string& propertyId);
    void notifyPropertyCatalogUpdated();
    
private:
    std::vector<PropertyUpdatedCallback> property_updated_callbacks_;
    std::vector<PropertyCatalogUpdatedCallback> property_catalog_updated_callbacks_;
    mutable std::recursive_mutex mutex_;
};

class ClientObservablePropertyList : public ObservablePropertyList {
public:
    using LoggerFunction = std::function<void(const std::string&, bool)>;
    
    ClientObservablePropertyList(LoggerFunction logger, MidiCIClientPropertyRules* property_client);
    ~ClientObservablePropertyList() override = default;
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const override;
    std::vector<PropertyValue> getValues() const override;

    void updateValue(const std::string& propertyId, const std::vector<uint8_t>& body, const std::string& mediaType = "application/json");
    std::string updateValue(const SubscribeProperty& msg);
    
private:
    LoggerFunction logger_;
    MidiCIClientPropertyRules* property_client_;
    std::map<std::string, PropertyValue> values_;
    mutable std::recursive_mutex mutex_;
};

class ServiceObservablePropertyList : public ObservablePropertyList {
public:
    using LoggerFunction = std::function<void(const std::string&, bool)>;
    
    ServiceObservablePropertyList(std::vector<PropertyValue>& internalValues, MidiCIServicePropertyRules& propertyService);
    ~ServiceObservablePropertyList() override = default;
    
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() const override;
    std::vector<PropertyValue> getValues() const override;

    // Safer method to get metadata by property ID without ownership transfer
    const PropertyMetadata* getMetadata(const std::string& property_id) const;
    
    void addMetadata(std::unique_ptr<PropertyMetadata> metadata);
    void updateMetadata(const std::string& propertyId, PropertyMetadata* metadata);
    void updateValue(const std::string& propertyId, const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    void updateValue(const std::string& propertyId, const std::string& resId, const std::string& mediaType, const std::vector<uint8_t>& body);
    void removeMetadata(const std::string& propertyId);
    
private:
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<PropertyValue>& internal_values_;
    midicci::commonproperties::MidiCIServicePropertyRules& property_service_;
    mutable std::recursive_mutex mutex_;
};

} // namespace
