#pragma once

#include <vector>
#include <string>
#include <map>
#include <functional>
#include <memory>
#include "../messages/Message.hpp"

namespace midi_ci {

namespace properties {

struct PropertyMetadata;
struct SubscriptionEntry;

class MidiCIServicePropertyRules {
public:
    virtual ~MidiCIServicePropertyRules() = default;
    
    virtual std::string get_property_id_for_header(const std::vector<uint8_t>& header) = 0;
    virtual std::vector<uint8_t> create_update_notification_header(const std::string& property_id, const std::map<std::string, std::string>& fields) = 0;
    virtual std::vector<PropertyMetadata> get_metadata_list() = 0;
    
    virtual messages::GetPropertyDataReply get_property_data(const messages::GetPropertyData& msg) = 0;
    virtual messages::SetPropertyDataReply set_property_data(const messages::SetPropertyData& msg) = 0;
    virtual messages::SubscribePropertyReply subscribe_property(const messages::SubscribeProperty& msg) = 0;
    
    virtual void add_metadata(const PropertyMetadata& property) = 0;
    virtual void remove_metadata(const std::string& property_id) = 0;
    
    virtual std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) = 0;
    virtual std::vector<uint8_t> decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) = 0;
    virtual std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) = 0;
    virtual std::vector<uint8_t> create_shutdown_subscription_header(const std::string& property_id) = 0;
    
    virtual const std::vector<SubscriptionEntry>& get_subscriptions() const = 0;
    
    void add_property_catalog_updated_callback(std::function<void()> callback);
    
protected:
    std::vector<std::function<void()>> property_catalog_updated_callbacks_;
};

struct PropertyMetadata {
    std::string property_id;
    std::string name;
    std::string description;
    std::string mime_type;
    std::vector<uint8_t> data;
    
    PropertyMetadata(const std::string& id, const std::string& n, const std::string& desc, const std::string& mime, const std::vector<uint8_t>& d)
        : property_id(id), name(n), description(desc), mime_type(mime), data(d) {}
};

struct SubscriptionEntry {
    std::string subscription_id;
    std::string property_id;
    uint32_t subscriber_muid;
    
    SubscriptionEntry(const std::string& sub_id, const std::string& prop_id, uint32_t muid)
        : subscription_id(sub_id), property_id(prop_id), subscriber_muid(muid) {}
};

} // namespace properties
} // namespace midi_ci
