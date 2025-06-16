#include "midi-ci/properties/CommonRulesPropertyClient.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/json/Json.hpp"
#include <sstream>
#include <algorithm>

namespace midi_ci {
namespace properties {

using namespace property_common_rules;

CommonRulesPropertyClient::CommonRulesPropertyClient(core::MidiCIDevice& device, core::ClientConnection& conn)
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
    json::JsonObject status_obj;
    status_obj[PropertyCommonHeaderKeys::STATUS] = json::JsonValue(status);
    
    json::JsonValue status_header(status_obj);
    auto json_str = status_header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyClient::encode_body(const std::vector<uint8_t>& data, const std::string& encoding) {
    return helper_->encode_body(data, encoding);
}

std::string CommonRulesPropertyClient::get_property_id_for_header(const std::vector<uint8_t>& header) {
    return helper_->get_property_identifier_internal(header);
}

std::string CommonRulesPropertyClient::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

int CommonRulesPropertyClient::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_integer(header, field);
}

void CommonRulesPropertyClient::process_property_subscription_result(void* sub, const messages::SubscribePropertyReply& msg) {
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
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&](const SubscriptionEntry& s) { return s.resource == property_id; });
    
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it);
    }
    
    subscriptions_.emplace_back(conn_.get_destination_id(), property_id, subscribe_id, "");
}

void CommonRulesPropertyClient::property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) {
    if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        auto list = get_metadata_list_for_body(body);
        resource_list_.clear();
        resource_list_.insert(resource_list_.end(), list.begin(), list.end());
        
        for (auto& callback : property_catalog_updated_callbacks_) {
            callback();
        }
        
        bool auto_send_device_info = true;
        if (auto_send_device_info) {
            auto it = std::find_if(resource_list_.begin(), resource_list_.end(),
                [](const PropertyMetadata& p) { return p.property_id == PropertyResourceNames::DEVICE_INFO; });
            if (it != resource_list_.end()) {
                conn_.get_property_client_facade().send_get_property_data(PropertyResourceNames::DEVICE_INFO, it->encoding);
            }
        }
    } else if (property_id == PropertyResourceNames::DEVICE_INFO) {
        try {
            json::JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);
            
            std::string manufacturer = json_body["manufacturer"].is_string() ? json_body["manufacturer"].as_string() : "";
            std::string family = json_body["family"].is_string() ? json_body["family"].as_string() : "";
            std::string model = json_body["model"].is_string() ? json_body["model"].as_string() : "";
            std::string version = json_body["version"].is_string() ? json_body["version"].as_string() : "";
            
            messages::DeviceInfo device_info(manufacturer, family, model, version);
            conn_.set_device_info(device_info);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::CHANNEL_LIST) {
        try {
            json::JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);
            conn_.set_channel_list(json_body);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::JSON_SCHEMA) {
        try {
            json::JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);
            conn_.set_json_schema(json_body);
        } catch (...) {
        }
    }
}

void CommonRulesPropertyClient::request_property_list(uint8_t group) {
    auto request_bytes = helper_->get_resource_list_request_bytes();
    
    messages::GetPropertyData msg(
        messages::Common(device_.get_muid(), conn_.get_destination_id(), 0x7F, group),
        0,
        request_bytes
    );
    
    conn_.get_property_client_facade().send_get_property_data(msg);
}

void CommonRulesPropertyClient::add_property_catalog_updated_callback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(callback);
}

std::vector<PropertyMetadata> CommonRulesPropertyClient::get_metadata_list() const {
    return resource_list_;
}

std::vector<PropertyMetadata> CommonRulesPropertyClient::get_metadata_list_for_body(const std::vector<uint8_t>& body) {
    try {
        json::JsonValue json_body;
        convert_application_json_bytes_to_json(body, json_body);
        
        std::vector<PropertyMetadata> result;
        
        if (json_body.is_array()) {
            const auto& array = json_body.as_array();
            for (size_t i = 0; i < array.size(); ++i) {
                const auto& entry = array[i];
                if (entry.is_object()) {
                    std::string resource = entry["resource"].is_string() ? entry["resource"].as_string() : "";
                    std::string can_set = entry["canSet"].is_string() ? entry["canSet"].as_string() : "";
                    std::string media_type = "application/json";
                    std::string encoding = "ASCII";
                    
                    if (entry["encodings"].is_array() && entry["encodings"].as_array().size() > 0) {
                        const auto& encodings_array = entry["encodings"].as_array();
                        encoding = encodings_array[0].is_string() ? encodings_array[0].as_string() : "ASCII";
                    }
                    
                    if (entry["mediaType"].is_array() && entry["mediaType"].as_array().size() > 0) {
                        const auto& media_type_array = entry["mediaType"].as_array();
                        media_type = media_type_array[0].is_string() ? media_type_array[0].as_string() : "application/json";
                    }
                    
                    result.emplace_back(resource, "", resource, media_type, encoding, std::vector<uint8_t>());
                }
            }
        }
        
        return result;
    } catch (...) {
        return std::vector<PropertyMetadata>();
    }
}

void CommonRulesPropertyClient::convert_application_json_bytes_to_json(const std::vector<uint8_t>& data, json::JsonValue& result) {
    std::string data_str(data.begin(), data.end());
    result = json::JsonValue::parse(data_str);
}

} // namespace properties
} // namespace midi_ci
