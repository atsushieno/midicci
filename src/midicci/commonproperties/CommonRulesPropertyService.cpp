#include "midicci/commonproperties/CommonRulesPropertyService.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include "midicci/PropertyCommonRules.hpp"
#include "midicci/Json.hpp"
#include "midicci/MidiCIConstants.hpp"
#include "midicci/MidiCIDeviceConfiguration.hpp"
#include <sstream>
#include <algorithm>

namespace midicci::commonproperties {

CommonRulesPropertyService::CommonRulesPropertyService(MidiCIDevice& device)
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

GetPropertyDataReply CommonRulesPropertyService::get_property_data(const GetPropertyData& msg) {
    std::string property_id = helper_->get_property_identifier_internal(msg.get_header());
    
    JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(property_id);
    
    std::vector<uint8_t> body_data;
    
    if (property_id == PropertyResourceNames::DEVICE_INFO) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_device_info_json();
    } else if (property_id == PropertyResourceNames::CHANNEL_LIST) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_channel_list_json();
    } else if (property_id == PropertyResourceNames::JSON_SCHEMA) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_json_schema_json();
    } else if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::OK);
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
        body_data = create_resource_list_json();
    } else {
        header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::RESOURCE_UNAVAILABLE_OR_ERROR);
        header_obj[PropertyCommonHeaderKeys::MESSAGE] = JsonValue("Property not found: " + property_id);
    }
    
    JsonValue header(header_obj);
    std::string json_str = header.serialize();
    std::vector<uint8_t> reply_header(json_str.begin(), json_str.end());

    auto& srcCommon = msg.get_common();
    Common common{device_.get_muid(), msg.get_source_muid(), srcCommon.address, srcCommon.group};
    return GetPropertyDataReply(common, msg.get_request_id(), reply_header, body_data);
}

std::vector<uint8_t> CommonRulesPropertyService::create_device_info_json() const {
    const auto& device_info = device_.get_device_info();
    
    JsonObject device_obj;
    device_obj[DeviceInfoPropertyNames::MANUFACTURER_ID] = JsonValue(static_cast<int>(device_info.manufacturer_id));
    device_obj[DeviceInfoPropertyNames::FAMILY_ID] = JsonValue(static_cast<int>(device_info.family_id));
    device_obj[DeviceInfoPropertyNames::MODEL_ID] = JsonValue(static_cast<int>(device_info.model_id));
    device_obj["versionId"] = JsonValue(static_cast<int>(device_info.version_id));
    device_obj["manufacturer"] = JsonValue(device_info.manufacturer);
    device_obj["family"] = JsonValue(device_info.family);
    device_obj["model"] = JsonValue(device_info.model);
    device_obj["version"] = JsonValue(device_info.version);
    device_obj["serialNumber"] = JsonValue(device_info.serial_number);
    
    JsonValue device_json(device_obj);
    std::string json_str = device_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyService::create_channel_list_json() const {
    const auto& channel_list = device_.get_config().channel_list;
    
    JsonArray channels_array;
    for (const auto& channel : channel_list.channels) {
        JsonObject channel_obj;
        channel_obj["title"] = JsonValue(channel.title);
        channel_obj["channel"] = JsonValue(channel.channel);
        channel_obj["programTitle"] = JsonValue(channel.program_title);
        channel_obj["bankMSB"] = JsonValue(static_cast<int>(channel.bank_msb));
        channel_obj["bankLSB"] = JsonValue(static_cast<int>(channel.bank_lsb));
        channel_obj["program"] = JsonValue(static_cast<int>(channel.program));
        channel_obj["clusterChannelStart"] = JsonValue(channel.cluster_channel_start);
        channel_obj["clusterLength"] = JsonValue(channel.cluster_length);
        channel_obj["isOmniOn"] = JsonValue(channel.is_omni_on);
        channel_obj["isPolyMode"] = JsonValue(channel.is_poly_mode);
        channel_obj["clusterType"] = JsonValue(channel.cluster_type);
        channels_array.push_back(JsonValue(channel_obj));
    }
    
    JsonValue channels_json(channels_array);
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
    JsonArray resources_array;
    
    for (const auto& property_id : get_property_ids()) {
        CommonRulesPropertyMetadata metadata(property_id);
        metadata.originator = CommonRulesPropertyMetadata::Originator::SYSTEM;
        resources_array.push_back(metadata.toJsonValue());
    }
    
    for (const auto& metadata : metadata_list_) {
        auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
        if (common_metadata) {
            resources_array.push_back(common_metadata->toJsonValue());
        } else {
            CommonRulesPropertyMetadata fallback_metadata(metadata->getPropertyId());
            fallback_metadata.originator = CommonRulesPropertyMetadata::Originator::USER;
            resources_array.push_back(fallback_metadata.toJsonValue());
        }
    }
    
    JsonValue resources_json(resources_array);
    std::string json_str = resources_json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

SetPropertyDataReply CommonRulesPropertyService::set_property_data(const SetPropertyData& msg) {
    return SetPropertyDataReply(msg.get_common(), msg.get_request_id(), {});
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

SubscribePropertyReply CommonRulesPropertyService::subscribe_property(const SubscribeProperty& msg) {
    return SubscribePropertyReply(msg.get_common(), msg.get_request_id(), {}, {});
}

std::string CommonRulesPropertyService::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

int CommonRulesPropertyService::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_integer(header, field);
}

std::vector<uint8_t> CommonRulesPropertyService::create_shutdown_subscription_header(const std::string& property_id) {
    std::map<std::string, std::string> fields;
    return helper_->create_request_header_bytes(property_id, fields);
}

const std::vector<SubscriptionEntry>& CommonRulesPropertyService::get_subscriptions() const {
    return subscriptions_;
}

} // namespace
