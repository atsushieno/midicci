#pragma once

#include "midicci/midicci.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace midicci {
namespace commonproperties {

struct PropertyCommonRequestHeader {
    std::string resource;
    std::string res_id;
    std::string mutual_encoding;
    std::string media_type;
    std::optional<int> offset;
    std::optional<int> limit;
    std::optional<bool> set_partial;
};

struct PropertyCommonReplyHeader {
    int status = 200;
    std::string message;
    std::string mutual_encoding;
    std::string media_type;
    std::string subscribe_id;
    std::optional<int> cache_time;
    std::optional<int> total_count;
};

class CommonRulesPropertyService : public MidiCIServicePropertyRules {
public:
    explicit CommonRulesPropertyService(MidiCIDevice& device);
    ~CommonRulesPropertyService() override = default;
    
    // MidiCIServicePropertyRules interface implementation
    std::string getPropertyIdForHeader(const std::vector<uint8_t>& header) override;
    std::vector<uint8_t> createUpdateNotificationHeader(const std::string& property_id, const std::map<std::string, std::string>& fields) override;
    std::vector<std::unique_ptr<PropertyMetadata>> getMetadataList() override;
    GetPropertyDataReply getPropertyData(const GetPropertyData& msg) override;
    SetPropertyDataReply setPropertyData(const SetPropertyData& msg) override;
    std::optional<SubscribePropertyReply> subscribeProperty(const SubscribeProperty& msg) override;
    void addMetadata(std::unique_ptr<PropertyMetadata> property) override;
    void removeMetadata(const std::string& property_id) override;
    std::vector<uint8_t> encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) override;
    std::vector<uint8_t> decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) override;
    std::string getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) override;
    int getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) override;
    std::vector<uint8_t> createShutdownSubscriptionHeader(const std::string& property_id, const std::string& res_id) override;
    const std::vector<SubscriptionEntry>& getSubscriptions() const override;
    
    // Additional helper methods
    void setPropertyValue(const std::string& property_id, const std::string& res_id,
                            const std::vector<uint8_t>& data,
                            const std::string& media_type = CommonRulesKnownMimeTypes::APPLICATION_JSON);

    // Property catalog update callback management (following Kotlin propertyCatalogUpdated)
    void addPropertyCatalogUpdatedCallback(std::function<void()> callback);
    void removePropertyCatalogUpdatedCallback(const std::function<void()>& callback);
    
    // Direct metadata access (for ServiceObservablePropertyList)
    const PropertyMetadata* getMetadataById(const std::string& property_id) const;
    
private:
    MidiCIDevice& device_;
    std::unique_ptr<CommonRulesPropertyHelper> helper_;
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<SubscriptionEntry> subscriptions_;
    
    // Property catalog update callbacks (following Kotlin propertyCatalogUpdated)
    std::vector<std::function<void()>> property_catalog_updated_callbacks_;
    
    // Subscription update callbacks (following Kotlin subscruotionsUpdated)
    std::vector<std::function<void(const SubscriptionEntry&, bool)>> subscription_updated_callbacks_;

public:
    std::function<std::vector<uint8_t>(const std::string& property_id, const std::string& res_id)> propertyBinaryGetter{};

    std::function<bool(const std::string& property_id, const std::string& res_id, const std::string& media_type, const std::vector<uint8_t>& body)> propertyBinarySetter{};

private:

    // Helper methods for subscription and property management
    PropertyCommonRequestHeader getPropertyHeader(const JsonValue& json) const;
    JsonValue getReplyHeaderJson(const PropertyCommonReplyHeader& src) const;
    std::string createNewSubscriptionId();
    std::pair<JsonValue, JsonValue> subscribe(uint32_t subscriber_muid, const JsonValue& header_json);
    std::pair<JsonValue, JsonValue> unsubscribe(const std::string& resource, const std::string& subscribe_id);

    JsonValue setPropertyData(const JsonValue& header_json, const std::vector<uint8_t>& body);
    std::pair<JsonValue, JsonValue> getPropertyDataJson(const PropertyCommonRequestHeader& header) const;
    std::pair<JsonValue, std::vector<uint8_t>> getPropertyDataEncoded(const JsonValue& header_json) const;
};

} // namespace properties
} // namespace midicci
