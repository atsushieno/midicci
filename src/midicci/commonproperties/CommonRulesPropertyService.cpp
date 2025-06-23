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
    try {
        std::string header_str(msg.get_header().begin(), msg.get_header().end());
        JsonValue header_json = JsonValue::parse(header_str);
        
        auto result = set_property_data_internal(header_json, msg.get_body());
        
        std::string reply_header_str = result.serialize();
        std::vector<uint8_t> reply_header(reply_header_str.begin(), reply_header_str.end());
        
        Common common{device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group};
        return SetPropertyDataReply(common, msg.get_request_id(), reply_header);
    } catch (const std::exception& e) {
        JsonObject error_header;
        error_header[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::INTERNAL_ERROR);
        error_header[PropertyCommonHeaderKeys::MESSAGE] = JsonValue("Error: " + std::string(e.what()));
        
        JsonValue error_json(error_header);
        std::string error_str = error_json.serialize();
        std::vector<uint8_t> reply_header(error_str.begin(), error_str.end());
        
        Common common{device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group};
        return SetPropertyDataReply(common, msg.get_request_id(), reply_header);
    }
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

std::optional<SubscribePropertyReply> CommonRulesPropertyService::subscribe_property(const SubscribeProperty& msg) {
    try {
        std::string header_str(msg.get_header().begin(), msg.get_header().end());
        JsonValue header_json = JsonValue::parse(header_str);
        
        std::string property_id = get_property_id_for_header(msg.get_header());
        std::string command = get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::COMMAND);
        
        JsonValue reply_header_json;
        JsonValue reply_body_json = JsonValue(JsonObject{});
        
        if (command == MidiCISubscriptionCommand::END) {
            std::string subscribe_id = get_header_field_string(msg.get_header(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
            auto result = unsubscribe(property_id, subscribe_id);
            reply_header_json = result.first;
            reply_body_json = result.second;
        } else {
            auto result = subscribe(msg.get_source_muid(), header_json);
            reply_header_json = result.first;
            reply_body_json = result.second;
        }
        
        std::string reply_header_str = reply_header_json.serialize();
        std::string reply_body_str = reply_body_json.serialize();
        
        std::vector<uint8_t> reply_header(reply_header_str.begin(), reply_header_str.end());
        std::vector<uint8_t> reply_body(reply_body_str.begin(), reply_body_str.end());
        
        Common common{device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group};
        return SubscribePropertyReply(common, msg.get_request_id(), reply_header, reply_body);
    } catch (const std::exception& e) {
        device_.get_logger()("Error processing SubscribeProperty: " + std::string(e.what()), true);
        return std::nullopt;
    }
}

std::string CommonRulesPropertyService::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

int CommonRulesPropertyService::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_integer(header, field);
}

std::vector<uint8_t> CommonRulesPropertyService::create_shutdown_subscription_header(const std::string& property_id) {
    auto e = std::find_if(subscriptions_.begin(), subscriptions_.end(), [&property_id](const SubscriptionEntry& entry) {
        return entry.resource == property_id;
    });
    if (e == subscriptions_.end()) return {}; // FIXME: should we throw an error instead?
    return helper_->create_subscribe_header_bytes(property_id, MidiCISubscriptionCommand::END);
}

const std::vector<SubscriptionEntry>& CommonRulesPropertyService::get_subscriptions() const {
    return subscriptions_;
}

PropertyCommonRequestHeader CommonRulesPropertyService::get_property_header(const JsonValue& json) {
    PropertyCommonRequestHeader header;
    
    if (json.is_object()) {
        const auto& obj = json.as_object();
        
        auto resource_it = obj.find(PropertyCommonHeaderKeys::RESOURCE);
        if (resource_it != obj.end() && resource_it->second.is_string()) {
            header.resource = resource_it->second.as_string();
        }
        
        auto res_id_it = obj.find(PropertyCommonHeaderKeys::RES_ID);
        if (res_id_it != obj.end() && res_id_it->second.is_string()) {
            header.res_id = res_id_it->second.as_string();
        }
        
        auto encoding_it = obj.find(PropertyCommonHeaderKeys::MUTUAL_ENCODING);
        if (encoding_it != obj.end() && encoding_it->second.is_string()) {
            header.mutual_encoding = encoding_it->second.as_string();
        }
        
        auto media_type_it = obj.find(PropertyCommonHeaderKeys::MEDIA_TYPE);
        if (media_type_it != obj.end() && media_type_it->second.is_string()) {
            header.media_type = media_type_it->second.as_string();
        }
        
        auto set_partial_it = obj.find(PropertyCommonHeaderKeys::SET_PARTIAL);
        if (set_partial_it != obj.end() && set_partial_it->second.is_bool()) {
            header.set_partial = set_partial_it->second.as_bool();
        }
    }
    
    return header;
}

JsonValue CommonRulesPropertyService::get_reply_header_json(const PropertyCommonReplyHeader& src) {
    JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(src.status);
    
    if (!src.message.empty()) {
        header_obj[PropertyCommonHeaderKeys::MESSAGE] = JsonValue(src.message);
    }
    
    if (!src.mutual_encoding.empty() && src.mutual_encoding != PropertyDataEncoding::ASCII) {
        header_obj[PropertyCommonHeaderKeys::MUTUAL_ENCODING] = JsonValue(src.mutual_encoding);
    }
    
    if (!src.media_type.empty()) {
        header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(src.media_type);
    }
    
    if (!src.subscribe_id.empty()) {
        header_obj[PropertyCommonHeaderKeys::SUBSCRIBE_ID] = JsonValue(src.subscribe_id);
    }
    
    return JsonValue(header_obj);
}

std::string CommonRulesPropertyService::create_new_subscription_id() {
    static int subscription_counter = 0;
    return std::to_string(++subscription_counter);
}

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::subscribe(uint32_t subscriber_muid, const JsonValue& header_json) {
    PropertyCommonRequestHeader header = get_property_header(header_json);
    
    std::string subscription_id = create_new_subscription_id();
    SubscriptionEntry subscription(
        subscriber_muid,
        header.resource,
        subscription_id,
        header.mutual_encoding.empty() ? PropertyDataEncoding::ASCII : header.mutual_encoding
    );
    
    subscriptions_.push_back(subscription);
    
    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    reply_header.subscribe_id = subscription_id;
    
    JsonValue reply_body = JsonValue(JsonObject{});
    
    return std::make_pair(get_reply_header_json(reply_header), reply_body);
}

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::unsubscribe(const std::string& resource, const std::string& subscribe_id) {
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&subscribe_id, &resource](const SubscriptionEntry& entry) {
            return (!subscribe_id.empty() && entry.subscribe_id == subscribe_id) ||
                   (subscribe_id.empty() && entry.resource == resource);
        });
    
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it);
    }
    
    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    reply_header.subscribe_id = subscribe_id;
    
    JsonValue reply_body = JsonValue(JsonObject{});
    
    return std::make_pair(get_reply_header_json(reply_header), reply_body);
}

JsonValue CommonRulesPropertyService::set_property_data_internal(const JsonValue& header_json, const std::vector<uint8_t>& body) {
    PropertyCommonRequestHeader header = get_property_header(header_json);
    
    // Check if it's a system property (read-only)
    if (header.resource == PropertyResourceNames::DEVICE_INFO ||
        header.resource == PropertyResourceNames::CHANNEL_LIST ||
        header.resource == PropertyResourceNames::JSON_SCHEMA ||
        header.resource == PropertyResourceNames::RESOURCE_LIST) {
        
        PropertyCommonReplyHeader reply_header;
        reply_header.status = PropertyExchangeStatus::INTERNAL_ERROR;
        reply_header.message = "Resource is readonly: " + header.resource;
        return get_reply_header_json(reply_header);
    }
    
    // Store the property value
    property_values_[header.resource] = body;
    
    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    return get_reply_header_json(reply_header);
}

} // namespace