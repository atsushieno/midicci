#pragma once

#include "MidiCIServicePropertyRules.hpp"
#include "CommonRulesPropertyHelper.hpp"
#include "../core/MidiCIDevice.hpp"
#include <memory>
#include <string>
#include <vector>
#include <map>

namespace midicci {
namespace propertycommonrules {

class CommonRulesPropertyService : public MidiCIServicePropertyRules {
public:
    explicit CommonRulesPropertyService(core::MidiCIDevice& device);
    ~CommonRulesPropertyService() override = default;
    
    // MidiCIServicePropertyRules interface implementation
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
    
    // Additional helper methods
    std::vector<std::string> get_property_ids() const;
    
private:
    core::MidiCIDevice& device_;
    std::unique_ptr<CommonRulesPropertyHelper> helper_;
    std::map<std::string, std::vector<uint8_t>> property_values_;
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<SubscriptionEntry> subscriptions_;
    
    std::vector<uint8_t> create_device_info_json() const;
    std::vector<uint8_t> create_channel_list_json() const;
    std::vector<uint8_t> create_json_schema_json() const;
    std::vector<uint8_t> create_resource_list_json() const;
};

} // namespace properties
} // namespace midicci
