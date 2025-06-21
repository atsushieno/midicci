#pragma once

#include "../Json.hpp"
#include "../MidiCIDevice.hpp"
#include "../ClientConnection.hpp"
#include "../PropertyClientFacade.hpp"
#include "CommonRulesPropertyHelper.hpp"
#include "../ObservablePropertyList.hpp"
#include <memory>
#include <vector>
#include <functional>

namespace midicci {
namespace commonproperties {

class CommonRulesPropertyClient : public MidiCIClientPropertyRules {
public:
    CommonRulesPropertyClient(MidiCIDevice& device, ClientConnection& conn);
    ~CommonRulesPropertyClient() override = default;
    
    std::vector<uint8_t> create_data_request_header(const std::string& resource, 
                                                   const std::map<std::string, std::string>& fields) override;
    
    std::vector<uint8_t> create_subscription_header(const std::string& resource, 
                                                   const std::map<std::string, std::string>& fields) override;
    
    std::vector<uint8_t> create_status_header(int status) override;
    
    std::vector<uint8_t> encode_body(const std::vector<uint8_t>& data, const std::string& encoding) override;
    
    std::string get_property_id_for_header(const std::vector<uint8_t>& header) override;
    
    std::string get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) override;
    
    int get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) override;
    
    void process_property_subscription_result(void* sub, const SubscribePropertyReply& msg) override;
    
    void property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) override;
    
    void request_property_list(uint8_t group) override;
    
    void add_property_catalog_updated_callback(std::function<void()> callback);
    
    std::vector<std::unique_ptr<PropertyMetadata>> get_metadata_list() const;

private:
    MidiCIDevice& device_;
    ClientConnection& conn_;
    std::unique_ptr<CommonRulesPropertyHelper> helper_;
    std::vector<std::function<void()>> property_catalog_updated_callbacks_;
    std::vector<std::unique_ptr<PropertyMetadata>> resource_list_;
    std::vector<SubscriptionEntry> subscriptions_;
    
    std::vector<std::unique_ptr<PropertyMetadata>> get_metadata_list_for_body(const std::vector<uint8_t>& body);
    void convert_application_json_bytes_to_json(const std::vector<uint8_t>& data, JsonValue& result);
};

} // namespace properties
} // namespace midi_ci
