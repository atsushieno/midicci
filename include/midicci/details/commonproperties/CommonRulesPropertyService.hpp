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
    std::string get_property_id_for_header(const std::vector<uint8_t>& header) override;
    std::vector<uint8_t> create_update_notification_header(const std::string& property_id, const std::map<std::string, std::string>& fields) override;
    std::vector<std::unique_ptr<PropertyMetadata>> get_metadata_list() override;
    GetPropertyDataReply get_property_data(const GetPropertyData& msg) override;
    SetPropertyDataReply set_property_data(const SetPropertyData& msg) override;
    std::optional<SubscribePropertyReply> subscribe_property(const SubscribeProperty& msg) override;
    void add_metadata(std::unique_ptr<PropertyMetadata> property) override;
    void remove_metadata(const std::string& property_id) override;
    std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) override;
    std::vector<uint8_t> decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) override;
    std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) override;
    int get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) override;
    std::vector<uint8_t> create_shutdown_subscription_header(const std::string& property_id, const std::string& res_id) override;
    const std::vector<SubscriptionEntry>& get_subscriptions() const override;
    
    // Additional helper methods
    void set_property_value(const std::string& property_id, const std::string& res_id,
                            const std::vector<uint8_t>& data,
                            const std::string& media_type = CommonRulesKnownMimeTypes::APPLICATION_JSON);

    // Property catalog update callback management (following Kotlin propertyCatalogUpdated)
    void add_property_catalog_updated_callback(std::function<void()> callback);
    void remove_property_catalog_updated_callback(const std::function<void()>& callback);
    
    // Direct metadata access (for ServiceObservablePropertyList)
    const PropertyMetadata* get_metadata_by_id(const std::string& property_id) const;
    
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
    PropertyCommonRequestHeader get_property_header(const JsonValue& json) const;
    JsonValue get_reply_header_json(const PropertyCommonReplyHeader& src) const;
    std::string create_new_subscription_id();
    std::pair<JsonValue, JsonValue> subscribe(uint32_t subscriber_muid, const JsonValue& header_json);
    std::pair<JsonValue, JsonValue> unsubscribe(const std::string& resource, const std::string& subscribe_id);

    JsonValue set_property_data(const JsonValue& header_json, const std::vector<uint8_t>& body);
    std::pair<JsonValue, JsonValue> get_property_data_json(const PropertyCommonRequestHeader& header) const;
    std::pair<JsonValue, std::vector<uint8_t>> get_property_data_encoded(const JsonValue& header_json) const;
};

} // namespace properties
} // namespace midicci
