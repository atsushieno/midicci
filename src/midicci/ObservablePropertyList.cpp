#include "midicci/ObservablePropertyList.hpp"
#include "midicci/PropertyClientFacade.hpp"
#include "midicci/commonproperties/CommonRulesPropertyService.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/commonproperties/CommonRulesPropertyClient.hpp"
#include "midicci/Message.hpp"
#include "midicci/PropertyCommonRules.hpp"
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

    ClientObservablePropertyList::ClientObservablePropertyList(LoggerFunction logger, MidiCIClientPropertyRules* property_client)
            : logger_(std::move(logger)), property_client_(property_client) {

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

    void ServiceObservablePropertyList::addMetadata(std::unique_ptr<PropertyMetadata> metadata) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Add to property service - this will trigger the catalog update callback we registered
        property_service_.add_metadata(std::move(metadata));
        // Note: Don't call notifyPropertyCatalogUpdated() here as it will be called by the callback
    }

    void ServiceObservablePropertyList::updateMetadata(const std::string& propertyId, PropertyMetadata* metadata) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // FIXME: implement
        /*
        property_service_.remove_metadata(propertyId);
        property_service_.add_metadata(std::make_unique(metadata));
         */
    }

    void ServiceObservablePropertyList::updateValue(const std::string& propertyId, const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto it = std::find_if(internal_values_.begin(), internal_values_.end(),[&](PropertyValue& i) { return i.id == propertyId; });
        if (it != internal_values_.end()) {
            it->body = body;
            notifyPropertyUpdated(propertyId);
        }
    }

    void ServiceObservablePropertyList::updateValue(const std::string& propertyId, const std::string& resId,
                                                    const std::string& mediaType, const std::vector<uint8_t>& body) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        auto it = std::find_if(internal_values_.begin(), internal_values_.end(),[&](PropertyValue& i) { return i.id == propertyId; });
        if (it != internal_values_.end()) {
            it->resId = resId;
            it->mediaType = mediaType;
            it->body = body;
            notifyPropertyUpdated(propertyId);
        }
    }

    void ServiceObservablePropertyList::removeMetadata(const std::string& propertyId) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);

        // Remove from property service - this will trigger the catalog update callback we registered
        property_service_.remove_metadata(propertyId);
        // Note: Don't call notifyPropertyCatalogUpdated() here as it will be called by the callback
    }

    SubscriptionEntry::SubscriptionEntry(uint32_t subscriber_muid, const std::string& res,
                                         const std::string& sub_id, const std::string& enc)
            : muid(subscriber_muid), resource(res), subscribe_id(sub_id), encoding(enc) {}

} // namespace
