#include "midicci/properties/ObservablePropertyList.hpp"
#include "midicci/properties/PropertyClientFacade.hpp"
#include "midicci/properties/CommonRulesPropertyMetadata.hpp"
#include "midicci/properties/CommonRulesPropertyClient.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace midicci {
namespace properties {

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
}

void ObservablePropertyList::removePropertyCatalogUpdatedCallback(const PropertyCatalogUpdatedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
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

ClientObservablePropertyList::ClientObservablePropertyList(LoggerFunction logger, MidiCIClientPropertyRules* property_client)
    : logger_(std::move(logger)), property_client_(property_client) {}

std::vector<std::unique_ptr<PropertyMetadata>> ClientObservablePropertyList::getMetadataList() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    auto* common_rules_client = dynamic_cast<CommonRulesPropertyClient*>(property_client_);
    if (common_rules_client) {
        return common_rules_client->get_metadata_list();
    }
    return {};
}

std::vector<PropertyValue> ClientObservablePropertyList::getValues() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<PropertyValue> result;
    for (const auto& pair : values_) {
        result.push_back(pair.second);
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

ServiceObservablePropertyList::ServiceObservablePropertyList(LoggerFunction logger)
    : logger_(std::move(logger)) {}

std::vector<std::unique_ptr<PropertyMetadata>> ServiceObservablePropertyList::getMetadataList() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    for (const auto& metadata : metadata_list_) {
        auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
        if (common_metadata) {
            result.push_back(std::make_unique<CommonRulesPropertyMetadata>(*common_metadata));
        }
    }
    return result;
}

std::vector<PropertyValue> ServiceObservablePropertyList::getValues() const {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<PropertyValue> result;
    for (const auto& pair : values_) {
        result.push_back(pair.second);
    }
    return result;
}

void ServiceObservablePropertyList::addProperty(std::unique_ptr<PropertyMetadata> metadata, const std::vector<uint8_t>& initialValue) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    const std::string propertyId = metadata->getPropertyId();
    metadata_list_.push_back(std::move(metadata));
    values_.emplace(propertyId, PropertyValue(propertyId, "application/json", initialValue));
    
    notifyPropertyCatalogUpdated();
    notifyPropertyUpdated(propertyId);
}

void ServiceObservablePropertyList::updateProperty(const std::string& propertyId, const std::vector<uint8_t>& body) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    auto it = values_.find(propertyId);
    if (it != values_.end()) {
        it->second.body = body;
        notifyPropertyUpdated(propertyId);
    }
}

void ServiceObservablePropertyList::removeProperty(const std::string& propertyId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    
    values_.erase(propertyId);
    metadata_list_.erase(
        std::remove_if(metadata_list_.begin(), metadata_list_.end(),
            [&propertyId](const std::unique_ptr<PropertyMetadata>& metadata) {
                return metadata->getPropertyId() == propertyId;
            }),
        metadata_list_.end()
    );
    
    notifyPropertyCatalogUpdated();
}

SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, 
                                   const std::string& sub_id, const std::string& enc)
    : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

} // namespace properties
} // namespace midi_ci
