#include "midicci/properties/CommonRulesPropertyService.hpp"
#include "midicci/properties/PropertyCommonRules.hpp"
#include "midicci/json_ish/Json.hpp"
#include "midicci/core/MidiCIConstants.hpp"
#include "midicci/core/MidiCIDeviceConfiguration.hpp"
#include <sstream>
#include <algorithm>

namespace midicci {
namespace properties {

using namespace property_common_rules;

CommonRulesPropertyService::CommonRulesPropertyService(core::MidiCIDevice& device)
    : device_(device), helper_(std::make_unique<CommonRulesPropertyHelper>(device)) {
}

std::vector<std::string> CommonRulesPropertyService::get_property_ids() const {
    return {
        PropertyResourceNames::DEVICE_INFO,
        PropertyResourceNames::CHANNEL_LIST,
        PropertyResourceNames::JSON_SCHEMA
    };
}

std::string CommonRulesPropertyService::get_property_id_for_header(const std::vector<uint8_t>& header) {
    return helper_->get_property_identifier_internal(header);
}

std::vector<uint8_t> CommonRulesPropertyService::create_update_notification_header(const std::string& property_id, const std::map<std::string, std::string>& fields) {
    return helper_->create_request_header_bytes(property_id, fields);
}

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyService::get_metadata_list() {
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    
    for (const auto& metadata : metadata_list_) {
        result.push_back(std::unique_ptr<PropertyMetadata>(metadata.get()));
    }
    
    return result;
}

messages::GetPropertyDataReply CommonRulesPropertyService::get_property_data(const messages::GetPropertyData& msg) {
    std::string property_id = helper_->get_property_identifier_internal(msg.get_header());
    
    json_ish::JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = json_ish::JsonValue(property_id);
    
    std::vector<uint8_t> body_data;
    
    if (property_id == PropertyResourceNames::DEVICE_INFO) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = json_ish::JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = json_ish::JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_device_info_json();
    } else if (property_id == PropertyResourceNames::CHANNEL_LIST) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = json_ish::JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = json_ish::JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_channel_list_json();
    } else if (property_id == PropertyResourceNames::JSON_SCHEMA) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = json_ish::JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = json_ish::JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_json_schema_json();
    } else if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = json_ish::JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = json_ish::JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_resource_list_json();
    } else {
        header_obj[PropertyCommonHeaderKeys::STATUS] = json_ish::JsonValue(PropertyExchangeStatus::RESOURCE_UNAVAILABLE_OR_ERROR);
        header_obj[PropertyCommonHeaderKeys::MESSAGE] = json_ish::JsonValue("Property not found: " + property_id);
    }
    
    json_ish::JsonValue header(header_obj);
    std::string json_str = header.serialize();
    std::vector<uint8_t> reply_header(json_str.begin(), json_str.end());
    
    return messages::GetPropertyDataReply(msg.get_common(), msg.get_request_id(), reply_header, body_data);
}

std::vector<uint8_t> CommonRulesPropertyService::create_device_info_json() const {
    const auto& device_info = device_.get_device_info();
    
    json_ish::JsonObject device_obj;
    device_obj[DeviceInfoPropertyNames::MANUFACTURER_ID] = json_ish::JsonValue(static_cast<int>(device_info.manufacturer_id));
    device_obj[DeviceInfoPropertyNames::FAMILY_ID] = json_ish::JsonValue(static_cast<int>(device_info.family_id));
    device_obj[DeviceInfoPropertyNames::MODEL_ID] = json_ish::JsonValue(static_cast<int>(device_info.model_id));
    device_obj["versionId"] = json_ish::JsonValue(static_cast<int>(device_info.version_id));
    device_obj["manufacturer"] = json_ish::JsonValue(device_info.manufacturer);
    device_obj["family"] = json_ish::JsonValue(device_info.family);
    device_obj["model"] = json_ish::JsonValue(device_info.model);
    device_obj["version"] = json_ish::JsonValue(device_info.version);
    device_obj["serialNumber"] = json_ish::JsonValue(device_info.serial_number);
    
    json_ish::JsonValue device_json(device_obj);
    std::string json_str = device_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyService::create_channel_list_json() const {
    const auto& channel_list = device_.get_config().channel_list;
    
    json_ish::JsonArray channels_array;
    for (const auto& channel : channel_list.channels) {
        json_ish::JsonObject channel_obj;
        channel_obj["title"] = json_ish::JsonValue(channel.title);
        channel_obj["channel"] = json_ish::JsonValue(channel.channel);
        channel_obj["programTitle"] = json_ish::JsonValue(channel.program_title);
        channel_obj["bankMSB"] = json_ish::JsonValue(static_cast<int>(channel.bank_msb));
        channel_obj["bankLSB"] = json_ish::JsonValue(static_cast<int>(channel.bank_lsb));
        channel_obj["program"] = json_ish::JsonValue(static_cast<int>(channel.program));
        channel_obj["clusterChannelStart"] = json_ish::JsonValue(channel.cluster_channel_start);
        channel_obj["clusterLength"] = json_ish::JsonValue(channel.cluster_length);
        channel_obj["isOmniOn"] = json_ish::JsonValue(channel.is_omni_on);
        channel_obj["isPolyMode"] = json_ish::JsonValue(channel.is_poly_mode);
        channel_obj["clusterType"] = json_ish::JsonValue(channel.cluster_type);
        channels_array.push_back(json_ish::JsonValue(channel_obj));
    }
    
    json_ish::JsonValue channels_json(channels_array);
    std::string json_str = channels_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyService::create_json_schema_json() const {
    const auto& json_schema_string = device_.get_config().json_schema_string;
    
    if (json_schema_string.empty()) {
        std::string empty_json = "{}";
        return std::vector<uint8_t>(empty_json.begin(), empty_json.end());
    }
    
    return std::vector<uint8_t>(json_schema_string.begin(), json_schema_string.end());
}

std::vector<uint8_t> CommonRulesPropertyService::create_resource_list_json() const {
    json_ish::JsonArray resources_array;
    
    for (const auto& property_id : get_property_ids()) {
        resources_array.push_back(json_ish::JsonValue(property_id));
    }
    
    for (const auto& metadata : metadata_list_) {
        resources_array.push_back(json_ish::JsonValue(metadata->getPropertyId()));
    }
    
    json_ish::JsonValue resources_json(resources_array);
    std::string json_str = resources_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

messages::SetPropertyDataReply CommonRulesPropertyService::set_property_data(const messages::SetPropertyData& msg) {
    return messages::SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
}

std::vector<uint8_t> CommonRulesPropertyService::encode_body(const std::vector<uint8_t>& data, const std::string& encoding) {
    return helper_->encode_body(data, encoding);
}

std::vector<uint8_t> CommonRulesPropertyService::decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    return helper_->decode_body(header, body);
}

void CommonRulesPropertyService::add_metadata(std::unique_ptr<PropertyMetadata> property) {
    metadata_list_.push_back(std::move(property));
}

void CommonRulesPropertyService::remove_metadata(const std::string& property_id) {
    metadata_list_.erase(
        std::remove_if(metadata_list_.begin(), metadata_list_.end(),
            [&property_id](const std::unique_ptr<PropertyMetadata>& metadata) {
                return metadata->getPropertyId() == property_id;
            }),
        metadata_list_.end());
    property_values_.erase(property_id);
}

messages::SubscribePropertyReply CommonRulesPropertyService::subscribe_property(const messages::SubscribeProperty& msg) {
    return messages::SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
}

std::string CommonRulesPropertyService::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

std::vector<uint8_t> CommonRulesPropertyService::create_shutdown_subscription_header(const std::string& property_id) {
    std::map<std::string, std::string> fields;
    return helper_->create_request_header_bytes(property_id, fields);
}

const std::vector<SubscriptionEntry>& CommonRulesPropertyService::get_subscriptions() const {
    return subscriptions_;
}

} // namespace properties
} // namespace midicci
