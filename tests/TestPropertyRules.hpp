#pragma once

#include "midi-ci/properties/ObservablePropertyList.hpp"
#include "midi-ci/properties/MidiCIServicePropertyRules.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"
#include "midi-ci/json/Json.hpp"
#include "midi-ci/messages/Message.hpp"
#include <unordered_map>

using namespace midi_ci::properties;
using namespace midi_ci::json;
using namespace midi_ci::messages;
using namespace midi_ci::properties::property_common_rules;

class TestPropertyRules : public MidiCIServicePropertyRules {
public:
    TestPropertyRules();
    
    std::string get_property_id_for_header(const std::vector<uint8_t>& header) override;
    std::vector<uint8_t> create_update_notification_header(const std::string& property_id, const std::map<std::string, std::string>& fields) override;
    std::vector<std::unique_ptr<PropertyMetadata>> get_metadata_list() override;
    
    GetPropertyDataReply get_property_data(const GetPropertyData& msg) override;
    SetPropertyDataReply set_property_data(const SetPropertyData& msg) override;
    SubscribePropertyReply subscribe_property(const SubscribeProperty& msg) override;
    
    void add_metadata(std::unique_ptr<PropertyMetadata> property) override;
    void remove_metadata(const std::string& property_id) override;
    
    std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) override;
    std::vector<uint8_t> decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) override;
    std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) override;
    std::vector<uint8_t> create_shutdown_subscription_header(const std::string& property_id) override;
    
    const std::vector<SubscriptionEntry>& get_subscriptions() const override;
    
    // Test-specific methods
    void set_property_value(const std::string& property_id, const std::vector<uint8_t>& data);
    std::vector<uint8_t> get_property_value(const std::string& property_id) const;
    
private:
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::unordered_map<std::string, std::vector<uint8_t>> property_values_;
    std::vector<SubscriptionEntry> subscriptions_;
    
    void initialize_default_properties();
    std::string parse_property_id_from_header(const std::vector<uint8_t>& header) const;
    std::vector<uint8_t> create_json_header(const std::string& property_id, const std::map<std::string, std::string>& fields = {}) const;
};
