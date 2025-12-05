#include "midicci/midicci.hpp"
#include <sstream>
#include <algorithm>
#include <random>

namespace midicci::commonproperties {

CommonRulesPropertyService::CommonRulesPropertyService(MidiCIDevice& device)
    : device_(device), helper_(std::make_unique<CommonRulesPropertyHelper>(device)) {
}


void CommonRulesPropertyService::set_property_value(const std::string& property_id, const std::vector<uint8_t>& data) {
    property_values_[property_id] = data;
}

void CommonRulesPropertyService::add_property_catalog_updated_callback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(std::move(callback));
}

void CommonRulesPropertyService::remove_property_catalog_updated_callback(const std::function<void()>& callback) {
    // For simplicity, we don't implement removal since std::function doesn't support comparison
    // In a real implementation, you might use a token-based system or store callbacks differently
}

const PropertyMetadata* CommonRulesPropertyService::get_metadata_by_id(const std::string& property_id) const {
    for (const auto& metadata : metadata_list_) {
        if (metadata && metadata->getPropertyId() == property_id) {
            return metadata.get();
        }
    }
    return nullptr;
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
        // Create copies of the metadata instead of raw pointers
        auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
        if (common_metadata) {
            result.push_back(std::make_unique<CommonRulesPropertyMetadata>(*common_metadata));
        }
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
        // Check if it's a user-defined property in our metadata list
        auto it = std::find_if(metadata_list_.begin(), metadata_list_.end(),
            [&property_id](const std::unique_ptr<PropertyMetadata>& metadata) {
                return metadata->getPropertyId() == property_id;
            });
        
        if (it != metadata_list_.end()) {
            // Found user-defined property - return its data
            header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::OK);
            header_obj[PropertyCommonHeaderKeys::MEDIA_TYPE] = JsonValue(CommonRulesKnownMimeTypes::APPLICATION_JSON);
            
            // Get the property data - first check if we have stored value, otherwise use metadata default
            auto value_it = property_values_.find(property_id);
            if (value_it != property_values_.end()) {
                body_data = value_it->second;
            } else {
                // Use the data from metadata
                body_data = (*it)->getData();
            }
        } else {
            header_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::RESOURCE_UNAVAILABLE_OR_ERROR);
            header_obj[PropertyCommonHeaderKeys::MESSAGE] = JsonValue("Property not found: " + property_id);
        }
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
    JsonValue json = FoundationalResources::toJsonValue(device_info);
    std::string json_str = json.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyService::create_channel_list_json() const {
    const auto& channel_list = device_.get_config().channel_list;
    JsonValue json = FoundationalResources::toJsonValue(channel_list);
    std::string json_str = json.serialize();
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
    std::vector<std::unique_ptr<PropertyMetadata>> all_metadata;
    
    // Add system properties
    std::vector<std::string> system_properties = {
        PropertyResourceNames::DEVICE_INFO,
        PropertyResourceNames::CHANNEL_LIST,
        PropertyResourceNames::JSON_SCHEMA
    };
    
    for (const auto& property_id : system_properties) {
        auto metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
        metadata->originator = CommonRulesPropertyMetadata::Originator::SYSTEM;
        all_metadata.push_back(std::move(metadata));
    }
    
    // Add user properties
    for (const auto& metadata : metadata_list_) {
        auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
        if (common_metadata) {
            all_metadata.push_back(std::make_unique<CommonRulesPropertyMetadata>(*common_metadata));
        } else {
            auto fallback_metadata = std::make_unique<CommonRulesPropertyMetadata>(metadata->getPropertyId());
            fallback_metadata->originator = CommonRulesPropertyMetadata::Originator::USER;
            all_metadata.push_back(std::move(fallback_metadata));
        }
    }
    
    JsonValue json = FoundationalResources::toJsonValue(all_metadata);
    std::string json_str = json.serialize();
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
    // Trigger property catalog updated callbacks (following Kotlin)
    for (const auto& callback : property_catalog_updated_callbacks_) {
        callback();
    }
}

void CommonRulesPropertyService::remove_metadata(const std::string& property_id) {
    metadata_list_.erase(
        std::remove_if(metadata_list_.begin(), metadata_list_.end(),
            [&property_id](const std::unique_ptr<PropertyMetadata>& metadata) {
                return metadata->getPropertyId() == property_id;
            }),
        metadata_list_.end());
    property_values_.erase(property_id);
    // Trigger property catalog updated callbacks (following Kotlin)
    for (const auto& callback : property_catalog_updated_callbacks_) {
        callback();
    }
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
        device_.get_logger()(LogData("Error processing SubscribeProperty: " + std::string(e.what()), true));
        return std::nullopt;
    }
}

std::string CommonRulesPropertyService::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_string(header, field);
}

int CommonRulesPropertyService::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->get_header_field_integer(header, field);
}

std::vector<uint8_t> CommonRulesPropertyService::create_shutdown_subscription_header(const std::string& property_id, const std::string& res_id) {
    auto e = std::find_if(subscriptions_.begin(), subscriptions_.end(), [&property_id, &res_id](const SubscriptionEntry& entry) {
        return entry.resource == property_id && (res_id.empty() || entry.res_id == res_id);
    });
    if (e == subscriptions_.end()) return {}; // FIXME: should we throw an error instead?
    return helper_->create_subscribe_header_bytes(property_id, MidiCISubscriptionCommand::END);
}

const std::vector<SubscriptionEntry>& CommonRulesPropertyService::get_subscriptions() const {
    return subscriptions_;
}

PropertyCommonRequestHeader CommonRulesPropertyService::get_property_header(const JsonValue& json) const {
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
        
        auto offset_it = obj.find(PropertyCommonHeaderKeys::OFFSET);
        if (offset_it != obj.end() && offset_it->second.is_number()) {
            header.offset = static_cast<int>(offset_it->second.as_number());
        }
        
        auto limit_it = obj.find(PropertyCommonHeaderKeys::LIMIT);
        if (limit_it != obj.end() && limit_it->second.is_number()) {
            header.limit = static_cast<int>(limit_it->second.as_number());
        }
        
        auto set_partial_it = obj.find(PropertyCommonHeaderKeys::SET_PARTIAL);
        if (set_partial_it != obj.end() && set_partial_it->second.is_bool()) {
            header.set_partial = set_partial_it->second.as_bool();
        }
    }
    
    return header;
}

JsonValue CommonRulesPropertyService::get_reply_header_json(const PropertyCommonReplyHeader& src) const {
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
    
    if (src.cache_time.has_value()) {
        header_obj[PropertyCommonHeaderKeys::CACHE_TIME] = JsonValue(static_cast<double>(src.cache_time.value()));
    }
    
    if (src.total_count.has_value()) {
        header_obj[PropertyCommonHeaderKeys::TOTAL_COUNT] = JsonValue(static_cast<double>(src.total_count.value()));
    }
    
    return JsonValue(header_obj);
}

std::string CommonRulesPropertyService::create_new_subscription_id() {
    // Following Kotlin Random.nextInt(100000000).toString()
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(0, 99999999);
    return std::to_string(dis(gen));
}

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::subscribe(uint32_t subscriber_muid, const JsonValue& header_json) {
    PropertyCommonRequestHeader header = get_property_header(header_json);
    
    std::string subscription_id = create_new_subscription_id();
    SubscriptionEntry subscription(
        subscriber_muid,
        header.resource,
        header.res_id,
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

    // Handle partial updates (following Kotlin implementation)
    if (header.set_partial.has_value() && header.set_partial.value()) {
        auto existing_it = property_values_.find(header.resource);
        if (existing_it == property_values_.end()) {
            device_.get_logger()(LogData("Partial update is specified but there is no existing value for property " + header.resource, true));
        } else {
            // For simplicity, just overwrite for now - full partial update logic would need JSON merging
            property_values_[header.resource] = body;
        }
    } else {
        // Use propertyBinarySetter for dynamic property value setting (following Kotlin d3fc841eb)
        bool success = propertyBinarySetter(header.resource, header.res_id,
                                           header.media_type.empty() ? CommonRulesKnownMimeTypes::APPLICATION_JSON : header.media_type,
                                           body);
        if (!success) {
            PropertyCommonReplyHeader reply_header;
            reply_header.status = PropertyExchangeStatus::INTERNAL_ERROR;
            reply_header.message = "Failed to set property: " + header.resource;
            return get_reply_header_json(reply_header);
        }
    }

    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    return get_reply_header_json(reply_header);
}

// Additional helper methods following Kotlin implementation

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::get_property_data_json(const PropertyCommonRequestHeader& header) const {
    JsonValue body;
    
    if (header.resource == PropertyResourceNames::RESOURCE_LIST) {
        std::vector<std::unique_ptr<PropertyMetadata>> all_metadata;
        
        // Add system properties
        std::vector<std::string> system_properties = {
            PropertyResourceNames::DEVICE_INFO,
            PropertyResourceNames::CHANNEL_LIST,
            PropertyResourceNames::JSON_SCHEMA
        };
        
        for (const auto& property_id : system_properties) {
            auto metadata = std::make_unique<CommonRulesPropertyMetadata>(property_id);
            metadata->originator = CommonRulesPropertyMetadata::Originator::SYSTEM;
            all_metadata.push_back(std::move(metadata));
        }
        
        // Add user properties
        for (const auto& metadata : metadata_list_) {
            auto* common_metadata = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata.get());
            if (common_metadata) {
                all_metadata.push_back(std::make_unique<CommonRulesPropertyMetadata>(*common_metadata));
            }
        }
        
        body = FoundationalResources::toJsonValue(all_metadata);
    } else if (header.resource == PropertyResourceNames::DEVICE_INFO) {
        body = FoundationalResources::toJsonValue(device_.get_device_info());
    } else if (header.resource == PropertyResourceNames::CHANNEL_LIST) {
        body = FoundationalResources::toJsonValue(device_.get_config().channel_list);
    } else if (header.resource == PropertyResourceNames::JSON_SCHEMA) {
        const auto& json_schema_string = device_.get_config().json_schema_string;
        if (!json_schema_string.empty()) {
            body = JsonValue::parse(json_schema_string);
        } else {
            body = JsonValue(); // null
        }
    } else {
        // User-defined property - use propertyBinaryGetter for dynamic retrieval
        auto binary = propertyBinaryGetter(header.resource, header.res_id);
        if (!binary.empty()) {
            std::string body_str(binary.begin(), binary.end());
            body = JsonValue::parse(body_str);
        } else {
            auto value_it = property_values_.find(header.resource);
            if (value_it != property_values_.end()) {
                if (!value_it->second.empty()) {
                    std::string body_str(value_it->second.begin(), value_it->second.end());
                    body = JsonValue::parse(body_str);
                } else {
                    body = JsonValue(JsonObject{});
                }
            } else {
                throw std::runtime_error("Unknown property: " + header.resource + " (resId: " + header.res_id + ")");
            }
        }
    }
    
    // Property list pagination (Common Rules for PE 6.6.2)
    JsonValue paginated_body = body;
    int total_count = -1;
    
    if (body.is_array() && header.offset.has_value()) {
        auto& array = body.as_array();
        total_count = static_cast<int>(array.size());
        
        size_t offset = static_cast<size_t>(header.offset.value());
        JsonArray paginated_array;
        
        if (offset < array.size()) {
            size_t end = array.size();
            if (header.limit.has_value()) {
                end = std::min(end, offset + static_cast<size_t>(header.limit.value()));
            }
            
            for (size_t i = offset; i < end; ++i) {
                paginated_array.push_back(array[i]);
            }
        }
        
        paginated_body = JsonValue(paginated_array);
    }
    
    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    reply_header.mutual_encoding = header.mutual_encoding;
    if (total_count >= 0) {
        reply_header.total_count = total_count;
    }
    
    return std::make_pair(get_reply_header_json(reply_header), paginated_body.is_null() ? JsonValue(JsonObject{}) : paginated_body);
}

std::pair<JsonValue, std::vector<uint8_t>> CommonRulesPropertyService::get_property_data_internal(const JsonValue& header_json) const {
    PropertyCommonRequestHeader header = get_property_header(header_json);
    
    if (header.media_type.empty() || header.media_type == CommonRulesKnownMimeTypes::APPLICATION_JSON) {
        auto result = get_property_data_json(header);
        std::string body_str = result.second.serialize();
        std::vector<uint8_t> body(body_str.begin(), body_str.end());
        std::vector<uint8_t> encoded_body = helper_->encode_body(body, header.mutual_encoding);
        return std::make_pair(result.first, encoded_body);
    } else {
        // Non-JSON media type - use propertyBinaryGetter for dynamic retrieval
        auto body = propertyBinaryGetter(header.resource, header.res_id);
        if (body.empty()) {
            auto value_it = property_values_.find(header.resource);
            if (value_it != property_values_.end()) {
                body = value_it->second;
            }
        }

        std::vector<uint8_t> encoded_body = helper_->encode_body(body, header.mutual_encoding);
        
        PropertyCommonReplyHeader reply_header;
        reply_header.status = PropertyExchangeStatus::OK;
        reply_header.mutual_encoding = header.mutual_encoding;
        
        return std::make_pair(get_reply_header_json(reply_header), encoded_body);
    }
}

std::vector<uint8_t> CommonRulesPropertyService::decode_body_internal(const std::string& mutual_encoding, const std::vector<uint8_t>& body) const {
    std::vector<uint8_t> encoding_vec(mutual_encoding.begin(), mutual_encoding.end());
    return helper_->decode_body(encoding_vec, body);
}

} // namespace