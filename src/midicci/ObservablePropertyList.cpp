#include "midicci/ObservablePropertyList.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include <algorithm>
#include <mutex>

namespace midicci {

// SubscriptionEntry implementation
SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                                   const std::string& sub_id, const std::string& enc)
    : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

// ObservablePropertyList base implementation
void ObservablePropertyList::addPropertyUpdatedCallback(PropertyUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    property_updated_callbacks_.push_back(std::move(callback));
}

void ObservablePropertyList::addPropertyCatalogUpdatedCallback(PropertyCatalogUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    property_catalog_updated_callbacks_.push_back(std::move(callback));
}

void ObservablePropertyList::removePropertyUpdatedCallback(const PropertyUpdatedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Note: This is a simplified removal - in practice, you'd need to compare function targets
    property_updated_callbacks_.clear(); // Simplified for now
}

void ObservablePropertyList::removePropertyCatalogUpdatedCallback(const PropertyCatalogUpdatedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // Note: This is a simplified removal - in practice, you'd need to compare function targets
    property_catalog_updated_callbacks_.clear(); // Simplified for now
}

void ObservablePropertyList::notifyPropertyUpdated(const std::string& propertyId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& callback : property_updated_callbacks_) {
        callback(propertyId);
    }
}

void ObservablePropertyList::notifyPropertyCatalogUpdated() {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    for (const auto& callback : property_catalog_updated_callbacks_) {
        callback();
    }
}

// ClientObservablePropertyList implementation
ClientObservablePropertyList::ClientObservablePropertyList(LoggerFunction logger, MidiCIClientPropertyRules* property_client)
    : logger_(std::move(logger)), property_client_(property_client) {}

std::vector<std::unique_ptr<PropertyMetadata>> ClientObservablePropertyList::getMetadataList() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    // This would typically query the property client for metadata
    // For now, return empty list
    return {};
}

std::vector<PropertyValue> ClientObservablePropertyList::getValues() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<PropertyValue> result;
    for (const auto& [id, value] : values_) {
        result.push_back(value);
    }
    return result;
}

void ClientObservablePropertyList::updateValue(const std::string& propertyId, const std::vector<uint8_t>& body, const std::string& mediaType) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = values_.find(propertyId);
    if (it != values_.end()) {
        it->second.body = body;
        it->second.mediaType = mediaType;
    } else {
        values_.emplace(propertyId, PropertyValue(propertyId, mediaType, body));
    }
    
    notifyPropertyUpdated(propertyId);
}

std::string ClientObservablePropertyList::updateValue(const SubscribeProperty& msg) {
    // This would process the SubscribeProperty message and update values accordingly
    // Implementation depends on the SubscribeProperty message structure
    logger_("ClientObservablePropertyList::updateValue from SubscribeProperty message", false);
    return "OK";
}

// ServiceObservablePropertyList implementation
ServiceObservablePropertyList::ServiceObservablePropertyList(LoggerFunction logger)
    : logger_(std::move(logger)) {}

std::vector<std::unique_ptr<PropertyMetadata>> ServiceObservablePropertyList::getMetadataList() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    
    // Create copies of the metadata for return
    for (const auto& metadata : metadata_list_) {
        auto copy = std::make_unique<commonproperties::CommonRulesPropertyMetadata>(metadata->getPropertyId());
        if (auto* common_rules = dynamic_cast<const commonproperties::CommonRulesPropertyMetadata*>(metadata.get())) {
            auto* copy_common = static_cast<commonproperties::CommonRulesPropertyMetadata*>(copy.get());
            copy_common->setData(common_rules->getData());
        }
        result.push_back(std::move(copy));
    }
    
    return result;
}

std::vector<PropertyValue> ServiceObservablePropertyList::getValues() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<PropertyValue> result;
    for (const auto& [id, value] : values_) {
        result.push_back(value);
    }
    return result;
}

void ServiceObservablePropertyList::addProperty(std::unique_ptr<PropertyMetadata> metadata, const std::vector<uint8_t>& initialValue) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const std::string property_id = metadata->getPropertyId();
    
    // Remove existing property with same ID
    removeProperty(property_id);
    
    // Add new metadata
    metadata_list_.push_back(std::move(metadata));
    
    // Add initial value
    values_.emplace(property_id, PropertyValue(property_id, "application/json", initialValue));
    
    logger_("Added property: " + property_id, false);
    notifyPropertyCatalogUpdated();
}

void ServiceObservablePropertyList::updateProperty(const std::string& propertyId, const std::vector<uint8_t>& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = values_.find(propertyId);
    if (it != values_.end()) {
        it->second.body = body;
        logger_("Updated property value: " + propertyId, false);
        notifyPropertyUpdated(propertyId);
    } else {
        logger_("Property not found for update: " + propertyId, true);
    }
}

void ServiceObservablePropertyList::removeProperty(const std::string& propertyId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    // Remove from metadata list
    auto metadata_it = std::remove_if(metadata_list_.begin(), metadata_list_.end(),
        [&propertyId](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == propertyId;
        });
    
    bool removed_metadata = metadata_it != metadata_list_.end();
    metadata_list_.erase(metadata_it, metadata_list_.end());
    
    // Remove from values
    auto values_it = values_.find(propertyId);
    bool removed_value = values_it != values_.end();
    if (removed_value) {
        values_.erase(values_it);
    }
    
    if (removed_metadata || removed_value) {
        logger_("Removed property: " + propertyId, false);
        notifyPropertyCatalogUpdated();
    }
}

} // namespace midicci