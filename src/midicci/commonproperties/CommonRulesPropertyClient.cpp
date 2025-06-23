#include "midicci/commonproperties/CommonRulesPropertyClient.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
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
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&](const SubscriptionEntry& s) { return s.resource == property_id; });
    
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it);
    }
    
    subscriptions_.emplace_back(conn_.get_target_muid(), property_id, subscribe_id, "");
}

void CommonRulesPropertyClient::property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) {
    if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        auto list = get_metadata_list_for_body(body);
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
    } else if (property_id == PropertyResourceNames::DEVICE_INFO) {
        try {
            JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);

            auto manufacturerId = json_body[DeviceInfoPropertyNames::MANUFACTURER_ID].as_int();
            auto familyId = json_body[DeviceInfoPropertyNames::FAMILY_ID].as_int();
            auto modelId = json_body[DeviceInfoPropertyNames::MODEL_ID].as_int();
            auto versionId = json_body[DeviceInfoPropertyNames::VERSION_ID].as_int();
            std::string manufacturer = json_body[DeviceInfoPropertyNames::MANUFACTURER].is_string() ? json_body[DeviceInfoPropertyNames::MANUFACTURER].as_string() : "";
            std::string family = json_body[DeviceInfoPropertyNames::FAMILY].is_string() ? json_body[DeviceInfoPropertyNames::FAMILY].as_string() : "";
            std::string model = json_body[DeviceInfoPropertyNames::MODEL].is_string() ? json_body[DeviceInfoPropertyNames::MODEL].as_string() : "";
            std::string version = json_body[DeviceInfoPropertyNames::VERSION].is_string() ? json_body[DeviceInfoPropertyNames::VERSION].as_string() : "";
            std::string serial = json_body[DeviceInfoPropertyNames::SERIAL_NUMBER].is_string() ? json_body[DeviceInfoPropertyNames::SERIAL_NUMBER].as_string() : "";

            DeviceInfo device_info(manufacturerId, familyId, modelId, versionId,
                                         manufacturer, family, model, version, serial);
            conn_.set_device_info(device_info);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::CHANNEL_LIST) {
        try {
            JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);
            conn_.set_channel_list(json_body);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::JSON_SCHEMA) {
        try {
            JsonValue json_body;
            convert_application_json_bytes_to_json(body, json_body);
            conn_.set_json_schema(json_body);
        } catch (...) {
        }
    }
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
        JsonValue json_body;
        convert_application_json_bytes_to_json(body, json_body);
        
        std::vector<std::unique_ptr<PropertyMetadata>> result;
        
        if (json_body.is_array()) {
            const auto& array = json_body.as_array();
            for (size_t i = 0; i < array.size(); ++i) {
                const auto& entry = array[i];
                if (entry.is_object()) {
                    std::string resource = entry["resource"].is_string() ? entry["resource"].as_string() : "";
                    
                    auto metadata = std::make_unique<CommonRulesPropertyMetadata>(resource);
                    
                    if (entry["canSet"].is_string()) {
                        metadata->canSet = entry["canSet"].as_string();
                    }
                    
                    if (entry["encodings"].is_array() && entry["encodings"].as_array().size() > 0) {
                        const auto& encodings_array = entry["encodings"].as_array();
                        std::vector<std::string> encodings;
                        for (const auto& enc : encodings_array) {
                            if (enc.is_string()) encodings.push_back(enc.as_string());
                        }
                        metadata->encodings = encodings;
                    }
                    
                    if (entry["mediaType"].is_array() && entry["mediaType"].as_array().size() > 0) {
                        const auto& media_type_array = entry["mediaType"].as_array();
                        std::vector<std::string> mediaTypes;
                        for (const auto& mt : media_type_array) {
                            if (mt.is_string()) mediaTypes.push_back(mt.as_string());
                        }
                        metadata->mediaTypes = mediaTypes;
                    }
                    
                    result.push_back(std::move(metadata));
                }
            }
        }
        
        return result;
    } catch (...) {
        return std::vector<std::unique_ptr<PropertyMetadata>>();
    }
}

void CommonRulesPropertyClient::convert_application_json_bytes_to_json(const std::vector<uint8_t>& data, JsonValue& result) {
    std::string data_str(data.begin(), data.end());
    result = JsonValue::parse(data_str);
}

} // namespace properties
} // namespace midi_ci
