#include "midicci/commonproperties/CommonRulesPropertyClient.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/commonproperties/FoundationalResources.hpp"
#include "midicci/PropertyCommonRules.hpp"
#include "midicci/MidiCIDevice.hpp"
#include "midicci/ClientConnection.hpp"
#include "midicci/Message.hpp"
#include "midicci/Json.hpp"
#include <sstream>
#include <algorithm>

namespace midicci {
namespace commonproperties {

CommonRulesPropertyClient::CommonRulesPropertyClient(MidiCIDevice& device, ClientConnection& conn)
    : device_(device), conn_(conn), helper_(std::make_unique<CommonRulesPropertyHelper>(device)) {}

std::vector<uint8_t> CommonRulesPropertyClient::create_data_request_header(
    const std::string& resource, const std::map<std::string, std::string>& fields) {
    return helper_->create_request_header_bytes(resource, fields);
}

std::vector<uint8_t> CommonRulesPropertyClient::create_subscription_header(
    const std::string& resource, const std::map<std::string, std::string>& fields) {
    
    auto command_it = fields.find(PropertyCommonHeaderKeys::COMMAND);
    auto encoding_it = fields.find(PropertyCommonHeaderKeys::MUTUAL_ENCODING);
    
    std::string command = (command_it != fields.end()) ? command_it->second : "";
    std::string encoding = (encoding_it != fields.end()) ? encoding_it->second : "";
    
    return helper_->create_subscribe_header_bytes(resource, command, encoding);
}

std::vector<uint8_t> CommonRulesPropertyClient::create_status_header(int status) {
    JsonObject status_obj;
    status_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(status);
    
    JsonValue status_header(status_obj);
    auto json_str = status_header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyClient::encode_body(const std::vector<uint8_t>& data, const std::string& encoding) {
    return helper_->encode_body(data, encoding);
}

std::string CommonRulesPropertyClient::get_property_id_for_header(const std::vector<uint8_t>& header) {
    return helper_->get_property_identifier_internal(header);
}

std::string CommonRulesPropertyClient::get_res_id_for_header(const std::vector<uint8_t>& header) {
    return helper_->get_header_field_string(header, PropertyCommonHeaderKeys::RES_ID);
}

std::string CommonRulesPropertyClient::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

int CommonRulesPropertyClient::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_integer(header, field);
}

void CommonRulesPropertyClient::process_property_subscription_result(void* sub, const SubscribePropertyReply& msg) {
    if (!sub) return;
    
    int status = get_header_field_integer(msg.get_header(), PropertyCommonHeaderKeys::STATUS);
    if (status != PropertyExchangeStatus::OK) {
        return;
    }
    
    std::string subscribe_id = get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
    if (subscribe_id.empty()) {
        return;
    }
    
    std::string* property_id_ptr = static_cast<std::string*>(sub);
    std::string property_id = *property_id_ptr;
    std::string res_id = get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::RES_ID);
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&](const SubscriptionEntry& s) { return s.resource == property_id && (res_id.empty() || s.res_id == res_id); });
    
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it);
    }
    
    subscriptions_.emplace_back(conn_.get_target_muid(), property_id, res_id, subscribe_id, "");
}

void CommonRulesPropertyClient::property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) {
    // Here we only care about Foundational Resources.
    // Other standard properties should be handled at ObservablePropertyList.valueUpdated.
    if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        try {
            auto list = FoundationalResources::parseResourceList(body);
            resource_list_.clear();
            for (auto& item : list) {
                resource_list_.push_back(std::move(item));
            }
            
            for (auto& callback : property_catalog_updated_callbacks_) {
                callback();
            }
            
            bool auto_send_device_info = true;
            if (auto_send_device_info) {
                auto it = std::find_if(resource_list_.begin(), resource_list_.end(),
                    [](const std::unique_ptr<PropertyMetadata>& p) { return p->getPropertyId() == PropertyResourceNames::DEVICE_INFO; });
                if (it != resource_list_.end()) {
                    conn_.get_property_client_facade().send_get_property_data(PropertyResourceNames::DEVICE_INFO, (*it)->getEncoding());
                }
            }
        } catch (const std::exception& ex) {
            device_.get_logger()("Error parsing resource list: " + std::string(ex.what()), true);
        }
    }
    // Note: DeviceInfo, ChannelList, and JsonSchema are now handled via FoundationalResources
    // and should be accessed through ObservablePropertyList extension methods
}

void CommonRulesPropertyClient::request_property_list(uint8_t group) {
    auto request_bytes = helper_->get_resource_list_request_bytes();
    
    GetPropertyData msg(
        Common(device_.get_muid(), conn_.get_target_muid(), 0x7F, group),
        0,
        request_bytes
    );
    
    conn_.get_property_client_facade().send_get_property_data(msg);
}

std::string CommonRulesPropertyClient::get_subscribed_property(const SubscribeProperty& msg) {
    auto subscribe_id = get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
    if (subscribe_id.empty()) {
        return "";
    }
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&](const SubscriptionEntry& subscription) {
            return subscription.subscribe_id == subscribe_id;
        });
    
    if (it == subscriptions_.end()) {
        return "";
    }
    
    return it->resource;
}

void CommonRulesPropertyClient::add_property_catalog_updated_callback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(callback);
}

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyClient::get_metadata_list() const {
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    for (const auto& metadata : resource_list_) {
        auto copy = std::make_unique<CommonRulesPropertyMetadata>();
        copy->resource = metadata->getPropertyId();
        if (auto* common_rules = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get())) {
            copy->canGet = common_rules->canGet;
            copy->canSet = common_rules->canSet;
            copy->canSubscribe = common_rules->canSubscribe;
            copy->requireResId = common_rules->requireResId;
            copy->mediaTypes = common_rules->mediaTypes;
            copy->encodings = common_rules->encodings;
            copy->schema = common_rules->schema;
            copy->canPaginate = common_rules->canPaginate;
            copy->originator = common_rules->originator;
        }
        result.push_back(std::move(copy));
    }
    return result;
}

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyClient::get_metadata_list_for_body(const std::vector<uint8_t>& body) {
    try {
        return FoundationalResources::parseResourceList(body);
    } catch (const std::exception& ex) {
        device_.get_logger()("Error parsing metadata list: " + std::string(ex.what()), true);
        return std::vector<std::unique_ptr<PropertyMetadata>>();
    }
}


} // namespace properties
} // namespace midi_ci
