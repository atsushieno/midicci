#include "midicci/midicci.hpp"
#include <mutex>
#include <vector>
#include <algorithm>

namespace midicci {

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

    ClientObservablePropertyList::ClientObservablePropertyList(MidiCIClientPropertyRules* property_client)
            : property_client_(property_client) {

        auto* common_rules_client = dynamic_cast<CommonRulesPropertyClient*>(property_client_);
        if (common_rules_client) {
            common_rules_client->add_property_catalog_updated_callback([this]() {
                std::lock_guard<std::recursive_mutex> lock(mutex_);

                auto metadata_list = getMetadataList();

                std::map<std::string, PropertyValue> new_values;

                for (const auto& metadata : metadata_list) {
                    const std::string& property_id = metadata->getPropertyId();

                    auto existing_it = values_.find(property_id);
                    if (existing_it != values_.end()) {
                        new_values.emplace(property_id, existing_it->second);
                    } else {
                        std::string media_type = metadata->getMediaType();
                        if (media_type.empty()) {
                            media_type = "application/json";
                        }
                        new_values.emplace(property_id, PropertyValue(property_id, media_type, std::vector<uint8_t>()));
                    }
                }

                values_ = std::move(new_values);

                notifyPropertyCatalogUpdated();
            });
        }
    }

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

    void ClientObservablePropertyList::setPropertyValue(const std::string& propertyId, const std::string& resId, 
                                                        const std::vector<uint8_t>& data, bool isPartial) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        
        // For client property list, we use a default media type of application/json
        // and don't handle partial updates (following the pattern from existing updateValue method)
        auto it = values_.find(propertyId);
        if (it != values_.end()) {
            it->second.resId = resId;
            it->second.body = data;
            it->second.mediaType = "application/json";
        } else {
            values_.emplace(propertyId, PropertyValue(propertyId, resId, "application/json", data));
        }

        notifyPropertyUpdated(propertyId);
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
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        if (!property_client_) {
            return "";
        }

        // Get property ID from subscription mapping
        auto property_id = property_client_->get_subscribed_property(msg);
        if (property_id.empty()) {
            return "";
        }

        auto command = property_client_->get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::COMMAND);

        // For NOTIFY commands, just return the command without updating property
        if (command == MidiCISubscriptionCommand::NOTIFY) {
            return command;
        }

        // For FULL and PARTIAL commands, update the property value
        auto media_type = property_client_->get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::MEDIA_TYPE);
        if (media_type.empty()) {
            media_type = CommonRulesKnownMimeTypes::APPLICATION_JSON;
        }

        // For now, we don't handle encoding, so just use body as-is
        // In full implementation, you'd call property_client_->decode_body(msg.get_header(), msg.get_body())

        updateValue(property_id, msg.get_body(), media_type);

        return command;
    }

    ServiceObservablePropertyList::ServiceObservablePropertyList(std::vector<PropertyValue>& internalValues, MidiCIServicePropertyRules& propertyService)
            : internal_values_(internalValues), property_service_(propertyService) {
        
        // Initialize catalog updated event handler (following Kotlin initializeCatalogUpdatedEvent pattern)
        auto* common_rules_service = dynamic_cast<CommonRulesPropertyService*>(&property_service_);
        if (common_rules_service) {
            common_rules_service->add_property_catalog_updated_callback([this]() {
                // Note: Don't update metadata_list_ here since ServiceObservablePropertyList
                // gets its metadata directly from the property service via get_metadata_list()
                // Just notify that the catalog has been updated
                notifyPropertyCatalogUpdated();
            });
        }
    }

    std::vector<std::unique_ptr<PropertyMetadata>> ServiceObservablePropertyList::getMetadataList() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        // Delegate to the property service to get the current metadata list
        return property_service_.get_metadata_list();
    }

    std::vector<PropertyValue> ServiceObservablePropertyList::getValues() const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        return internal_values_;
    }

    const PropertyMetadata* ServiceObservablePropertyList::getMetadata(const std::string& property_id) const {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Use the proper accessor method from CommonRulesPropertyService
        auto* common_rules_service = dynamic_cast<CommonRulesPropertyService*>(&property_service_);
        if (common_rules_service) {
            return common_rules_service->get_metadata_by_id(property_id);
        }

        return nullptr;
    }

    std::vector<PropertyValue>& ServiceObservablePropertyList::getMutableValues() {
        return internal_values_;
    }

    void ServiceObservablePropertyList::setPropertyValue(const std::string& propertyId, const std::string& resId, 
                                                         const std::vector<uint8_t>& data, bool isPartial) {
        // For service property list, delegate to the existing updateValue method
        // Use default media type of application/json, and ignore the isPartial parameter for now
        updateValue(propertyId, resId, "application/json", data);
    }

    void ServiceObservablePropertyList::addMetadata(std::unique_ptr<PropertyMetadata> metadata) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Add to property service - this will trigger the catalog update callback we registered
        property_service_.add_metadata(std::move(metadata));
        // Note: Don't call notifyPropertyCatalogUpdated() here as it will be called by the callback
    }

    void ServiceObservablePropertyList::updateMetadata(const std::string& propertyId, PropertyMetadata* metadata) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Following Kotlin implementation: remove old property, then add new property
        // First, preserve the current property data (value) if it exists
        std::vector<uint8_t> existing_data;
        auto* old_metadata = getMetadata(propertyId);
        if (old_metadata) {
            existing_data = old_metadata->getData();
        }

        // Remove the old metadata from the service
        property_service_.remove_metadata(propertyId);

        // Create a new metadata object by copying from the provided metadata
        // We need to create a new CommonRulesPropertyMetadata with the updated values
        auto* common_rules_metadata = dynamic_cast<CommonRulesPropertyMetadata*>(metadata);
        if (common_rules_metadata) {
            auto new_metadata = std::make_unique<CommonRulesPropertyMetadata>();
            new_metadata->resource = common_rules_metadata->resource;
            new_metadata->canGet = common_rules_metadata->canGet;
            new_metadata->canSet = common_rules_metadata->canSet;
            new_metadata->canSubscribe = common_rules_metadata->canSubscribe;
            new_metadata->requireResId = common_rules_metadata->requireResId;
            new_metadata->canPaginate = common_rules_metadata->canPaginate;
            new_metadata->mediaTypes = common_rules_metadata->mediaTypes;
            new_metadata->encodings = common_rules_metadata->encodings;
            new_metadata->schema = common_rules_metadata->schema;
            new_metadata->originator = common_rules_metadata->originator;

            // Preserve the existing data if we had it, otherwise use the new metadata's data
            if (!existing_data.empty()) {
                new_metadata->data = existing_data;
            } else {
                new_metadata->data = common_rules_metadata->data;
            }

            // Add the new metadata to the service
            property_service_.add_metadata(std::move(new_metadata));
        }
        // Note: The property_catalog_updated callback will be triggered by add_metadata,
        // which will in turn call notifyPropertyCatalogUpdated()
    }

    void ServiceObservablePropertyList::updateValue(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        std::string propertyId = property_service_.get_property_id_for_header(header);
        std::string resId = property_service_.get_header_field_string(header, PropertyCommonHeaderKeys::RES_ID);
        std::string mediaType = property_service_.get_header_field_string(header, PropertyCommonHeaderKeys::MEDIA_TYPE);
        if (mediaType.empty())
            mediaType = CommonRulesKnownMimeTypes::APPLICATION_JSON;
        std::vector<uint8_t> decodedBody = property_service_.decode_body(header, body);
        updateValue(propertyId, resId, mediaType, decodedBody);
    }

    void ServiceObservablePropertyList::updateValue(const std::string& propertyId, const std::string& resId,
                                                    const std::string& mediaType, const std::vector<uint8_t>& body) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Following Kotlin implementation: remove existing property, then add new one
        auto it = std::find_if(internal_values_.begin(), internal_values_.end(),
            [&propertyId](const PropertyValue& pv) { return pv.id == propertyId; });
        if (it != internal_values_.end()) {
            internal_values_.erase(it);
        }
        
        // Add the new property value
        internal_values_.emplace_back(propertyId, resId, mediaType, body);
        
        // Notify that the property was updated
        notifyPropertyUpdated(propertyId);
    }

    void ServiceObservablePropertyList::removeMetadata(const std::string& propertyId) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Remove from property service - this will trigger the catalog update callback we registered
        property_service_.remove_metadata(propertyId);
        // Note: Don't call notifyPropertyCatalogUpdated() here as it will be called by the callback
    }

    SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res, const std::string& resource_id,
                                         const std::string& sub_id, const std::string& enc)
            : muid(subscriber_muid), resource(res), res_id(resource_id), subscribe_id(sub_id), encoding(enc) {}

} // namespace
