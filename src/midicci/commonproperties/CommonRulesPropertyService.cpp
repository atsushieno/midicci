#include "midicci/midicci.hpp"
#include "midicci/details/PropertyPartialUpdater.hpp"
#include <sstream>
#include <algorithm>
#include <random>

namespace midicci::commonproperties {

CommonRulesPropertyService::CommonRulesPropertyService(MidiCIDevice& device)
    : device_(device), helper_(std::make_unique<CommonRulesPropertyHelper>(device)) {
    propertyBinaryGetter = [this](const std::string& property_id, const std::string& res_id) -> std::vector<uint8_t> {
        const auto& values = device_.getConfig().property_values;
        auto it = std::find_if(values.begin(), values.end(),
            [&property_id, &res_id](const PropertyValue& pv) {
                return pv.id == property_id && (res_id.empty() || pv.resId == res_id);
            });
        if (it != values.end()) {
            return it->body;
        }
        return {};
    };

    propertyBinarySetter = [this](const std::string& property_id, const std::string& res_id, const std::string& media_type, const std::vector<uint8_t>& body) -> bool {
        auto& values = device_.getConfig().property_values;
        auto it = std::find_if(values.begin(), values.end(),
            [&property_id, &res_id](const PropertyValue& pv) {
                return pv.id == property_id && pv.resId == res_id;
            });

        if (it != values.end()) {
            it->body = body;
            it->mediaType = media_type;
            it->resId = res_id;
            return true;
        } else {
            device_.getConfig().property_values.push_back(PropertyValue(property_id, res_id, media_type, body));
            return true;
        }
    };
}
void CommonRulesPropertyService::setPropertyValue(const std::string& property_id, const std::string& res_id,
                                                    const std::vector<uint8_t>& data, const std::string& media_type) {
    auto& values = device_.getConfig().property_values;

    // Find the property value, but don't use iterator after potential vector modification
    // to avoid iterator invalidation in MSVC debug mode
    auto it = std::find_if(values.begin(), values.end(),
        [&property_id, &res_id](const PropertyValue& pv) {
            return pv.id == property_id && pv.resId == res_id;
        });

    bool found = (it != values.end());

    // Invalidate iterator before any operations that might trigger callbacks
    it = values.end();

    if (found) {
        // Find again and update (safe even if vector was modified)
        auto update_it = std::find_if(values.begin(), values.end(),
            [&property_id, &res_id](const PropertyValue& pv) {
                return pv.id == property_id && pv.resId == res_id;
            });
        if (update_it != values.end()) {
            update_it->body = data;
            update_it->mediaType = media_type;
            update_it->resId = res_id;
        }
    } else {
        device_.getConfig().property_values.push_back(
            PropertyValue(property_id, res_id, media_type, data));
    }
}

void CommonRulesPropertyService::addPropertyCatalogUpdatedCallback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(std::move(callback));
}

void CommonRulesPropertyService::removePropertyCatalogUpdatedCallback(const std::function<void()>& callback) {
    // For simplicity, we don't implement removal since std::function doesn't support comparison
    // In a real implementation, you might use a token-based system or store callbacks differently
}

const PropertyMetadata* CommonRulesPropertyService::getMetadataById(const std::string& property_id) const {
    for (const auto& metadata : metadata_list_) {
        if (metadata && metadata->getPropertyId() == property_id) {
            return metadata.get();
        }
    }
    return nullptr;
}

std::string CommonRulesPropertyService::getPropertyIdForHeader(const std::vector<uint8_t>& header) {
    return helper_->getPropertyIdentifierInternal(header);
}

std::vector<uint8_t> CommonRulesPropertyService::createUpdateNotificationHeader(const std::string& property_id, const std::map<std::string, std::string>& fields) {
    return helper_->createRequestHeaderBytes(property_id, fields);
}

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyService::getMetadataList() {
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

GetPropertyDataReply CommonRulesPropertyService::getPropertyData(const GetPropertyData& msg) {
    try {
        std::string header_str(msg.getHeader().begin(), msg.getHeader().end());
        JsonValue json_inquiry = JsonValue::parse(header_str);

        auto result = getPropertyDataEncoded(json_inquiry);

        std::string reply_header_str = result.first.serialize();
        std::vector<uint8_t> reply_header(reply_header_str.begin(), reply_header_str.end());
        const std::vector<uint8_t>& reply_body = result.second;

        auto& srcCommon = msg.getCommon();
        Common common{device_.getMuid(), msg.getSourceMuid(), srcCommon.address, srcCommon.group};
        return GetPropertyDataReply(common, msg.getRequestId(), reply_header, reply_body);
    } catch (const std::exception& e) {
        JsonObject error_header;
        error_header[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::INTERNAL_ERROR);
        error_header[PropertyCommonHeaderKeys::MESSAGE] = JsonValue("Error: " + std::string(e.what()));

        JsonValue error_json(error_header);
        std::string error_str = error_json.serialize();
        std::vector<uint8_t> reply_header(error_str.begin(), error_str.end());

        auto& srcCommon = msg.getCommon();
        Common common{device_.getMuid(), msg.getSourceMuid(), srcCommon.address, srcCommon.group};
        return GetPropertyDataReply(common, msg.getRequestId(), reply_header, std::vector<uint8_t>());
    }
}

SetPropertyDataReply CommonRulesPropertyService::setPropertyData(const SetPropertyData& msg) {
    try {
        std::string header_str(msg.getHeader().begin(), msg.getHeader().end());
        JsonValue header_json = JsonValue::parse(header_str);

        auto result = setPropertyData(header_json, msg.getBody());

        std::string reply_header_str = result.serialize();
        std::vector<uint8_t> reply_header(reply_header_str.begin(), reply_header_str.end());

        Common common{device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group};
        return SetPropertyDataReply(common, msg.getRequestId(), reply_header);
    } catch (const std::exception& e) {
        JsonObject error_header;
        error_header[PropertyCommonHeaderKeys::STATUS] = JsonValue(PropertyExchangeStatus::INTERNAL_ERROR);
        error_header[PropertyCommonHeaderKeys::MESSAGE] = JsonValue("Error: " + std::string(e.what()));

        JsonValue error_json(error_header);
        std::string error_str = error_json.serialize();
        std::vector<uint8_t> reply_header(error_str.begin(), error_str.end());

        Common common{device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group};
        return SetPropertyDataReply(common, msg.getRequestId(), reply_header);
    }
}

std::vector<uint8_t> CommonRulesPropertyService::encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) {
    return helper_->encodeBody(data, encoding);
}

std::vector<uint8_t> CommonRulesPropertyService::decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    return helper_->decodeBody(header, body);
}

void CommonRulesPropertyService::addMetadata(std::unique_ptr<PropertyMetadata> property) {
    metadata_list_.push_back(std::move(property));
    // Trigger property catalog updated callbacks (following Kotlin)
    for (const auto& callback : property_catalog_updated_callbacks_) {
        callback();
    }
}

void CommonRulesPropertyService::removeMetadata(const std::string& property_id) {
    metadata_list_.erase(
        std::remove_if(metadata_list_.begin(), metadata_list_.end(),
            [&property_id](const std::unique_ptr<PropertyMetadata>& metadata) {
                return metadata->getPropertyId() == property_id;
            }),
        metadata_list_.end());

    auto& values = device_.getConfig().property_values;
    values.erase(
        std::remove_if(values.begin(), values.end(),
            [&property_id](const PropertyValue& pv) {
                return pv.id == property_id;
            }),
        values.end());

    // Trigger property catalog updated callbacks (following Kotlin)
    for (const auto& callback : property_catalog_updated_callbacks_) {
        callback();
    }
}

std::optional<SubscribePropertyReply> CommonRulesPropertyService::subscribeProperty(const SubscribeProperty& msg) {
    try {
        std::string header_str(msg.getHeader().begin(), msg.getHeader().end());
        JsonValue header_json = JsonValue::parse(header_str);
        
        std::string property_id = getPropertyIdForHeader(msg.getHeader());
        std::string command = getHeaderFieldString(msg.getHeader(), PropertyCommonHeaderKeys::COMMAND);
        
        JsonValue reply_header_json;
        JsonValue reply_body_json = JsonValue(JsonObject{});
        
        if (command == MidiCISubscriptionCommand::END) {
            std::string subscribe_id = getHeaderFieldString(msg.getHeader(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
            auto result = unsubscribe(property_id, subscribe_id);
            reply_header_json = result.first;
            reply_body_json = result.second;
        } else {
            auto result = subscribe(msg.getSourceMuid(), header_json);
            reply_header_json = result.first;
            reply_body_json = result.second;
        }
        
        std::string reply_header_str = reply_header_json.serialize();
        std::string reply_body_str = reply_body_json.serialize();
        
        std::vector<uint8_t> reply_header(reply_header_str.begin(), reply_header_str.end());
        std::vector<uint8_t> reply_body(reply_body_str.begin(), reply_body_str.end());
        
        Common common{device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group};
        return SubscribePropertyReply(common, msg.getRequestId(), reply_header, reply_body);
    } catch (const std::exception& e) {
        device_.getLogger()(LogData("Error processing SubscribeProperty: " + std::string(e.what()), true));
        return std::nullopt;
    }
}

std::string CommonRulesPropertyService::getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->getHeaderFieldString(header, field);
}

int CommonRulesPropertyService::getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->getHeaderFieldInteger(header, field);
}

std::vector<uint8_t> CommonRulesPropertyService::createShutdownSubscriptionHeader(const std::string& property_id, const std::string& res_id) {
    auto e = std::find_if(subscriptions_.begin(), subscriptions_.end(), [&property_id, &res_id](const SubscriptionEntry& entry) {
        return entry.resource == property_id && (res_id.empty() || entry.res_id == res_id);
    });
    if (e == subscriptions_.end()) return {}; // FIXME: should we throw an error instead?
    return helper_->createSubscribeHeaderBytes(property_id, MidiCISubscriptionCommand::END);
}

const std::vector<SubscriptionEntry>& CommonRulesPropertyService::getSubscriptions() const {
    return subscriptions_;
}

PropertyCommonRequestHeader CommonRulesPropertyService::getPropertyHeader(const JsonValue& json) const {
    PropertyCommonRequestHeader header;
    
    if (json.isObject()) {
        const auto& obj = json.asObject();
        
        auto resource_it = obj.find(PropertyCommonHeaderKeys::RESOURCE);
        if (resource_it != obj.end() && resource_it->second.isString()) {
            header.resource = resource_it->second.asString();
        }
        
        auto res_id_it = obj.find(PropertyCommonHeaderKeys::RES_ID);
        if (res_id_it != obj.end() && res_id_it->second.isString()) {
            header.res_id = res_id_it->second.asString();
        }
        
        auto encoding_it = obj.find(PropertyCommonHeaderKeys::MUTUAL_ENCODING);
        if (encoding_it != obj.end() && encoding_it->second.isString()) {
            header.mutual_encoding = encoding_it->second.asString();
        }
        
        auto media_type_it = obj.find(PropertyCommonHeaderKeys::MEDIA_TYPE);
        if (media_type_it != obj.end() && media_type_it->second.isString()) {
            header.media_type = media_type_it->second.asString();
        }
        
        auto offset_it = obj.find(PropertyCommonHeaderKeys::OFFSET);
        if (offset_it != obj.end() && offset_it->second.isNumber()) {
            header.offset = static_cast<int>(offset_it->second.asNumber());
        }
        
        auto limit_it = obj.find(PropertyCommonHeaderKeys::LIMIT);
        if (limit_it != obj.end() && limit_it->second.isNumber()) {
            header.limit = static_cast<int>(limit_it->second.asNumber());
        }
        
        auto set_partial_it = obj.find(PropertyCommonHeaderKeys::SET_PARTIAL);
        if (set_partial_it != obj.end() && set_partial_it->second.isBool()) {
            header.set_partial = set_partial_it->second.asBool();
        }
    }
    
    return header;
}

JsonValue CommonRulesPropertyService::getReplyHeaderJson(const PropertyCommonReplyHeader& src) const {
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

std::string CommonRulesPropertyService::createNewSubscriptionId() {
    // Following Kotlin Random.nextInt(100000000).toString()
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int> dis(0, 99999999);
    return std::to_string(dis(gen));
}

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::subscribe(uint32_t subscriber_muid, const JsonValue& header_json) {
    PropertyCommonRequestHeader header = getPropertyHeader(header_json);
    
    std::string subscription_id = createNewSubscriptionId();
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
    
    return std::make_pair(getReplyHeaderJson(reply_header), reply_body);
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
    
    return std::make_pair(getReplyHeaderJson(reply_header), reply_body);
}

JsonValue CommonRulesPropertyService::setPropertyData(const JsonValue& header_json, const std::vector<uint8_t>& body) {
    PropertyCommonRequestHeader header = getPropertyHeader(header_json);

    if (header.resource == PropertyResourceNames::DEVICE_INFO ||
        header.resource == PropertyResourceNames::CHANNEL_LIST ||
        header.resource == PropertyResourceNames::JSON_SCHEMA ||
        header.resource == PropertyResourceNames::RESOURCE_LIST) {

        PropertyCommonReplyHeader reply_header;
        reply_header.status = PropertyExchangeStatus::INTERNAL_ERROR;
        reply_header.message = "Resource is readonly: " + header.resource;
        return getReplyHeaderJson(reply_header);
    }

    std::optional<std::string> mutual_encoding = header.mutual_encoding.empty()
        ? std::nullopt
        : std::optional<std::string>(header.mutual_encoding);
    std::vector<uint8_t> decoded_body = helper_->decodeBody(mutual_encoding, body);

    auto& values = device_.getConfig().property_values;
    auto existing_it = std::find_if(values.begin(), values.end(),
        [&header](const PropertyValue& pv) {
            return pv.id == header.resource;
        });

    if (header.set_partial.has_value() && header.set_partial.value()) {
        if (existing_it == values.end()) {
            device_.getLogger()(LogData("Partial update is specified but there is no existing value for property " + header.resource, true));
        } else {
            try {
                std::string decoded_str(decoded_body.begin(), decoded_body.end());
                JsonValue body_json = JsonValue::parse(decoded_str);

                std::string existing_str(existing_it->body.begin(), existing_it->body.end());
                JsonValue existing_json = JsonValue::parse(existing_str);

                auto result = PropertyPartialUpdater::applyPartialUpdates(existing_json, body_json);
                if (!result.first) {
                    device_.getLogger()(LogData("Failed partial update for property " + header.resource, true));
                } else {
                    std::string updated_str = result.second.serialize();
                    existing_it->body = std::vector<uint8_t>(updated_str.begin(), updated_str.end());
                }
            } catch (const std::exception& e) {
                device_.getLogger()(LogData("Error parsing JSON for partial update: " + std::string(e.what()), true));
            }
        }
    } else {
        bool success = propertyBinarySetter(header.resource, header.res_id,
                                           header.media_type.empty() ? CommonRulesKnownMimeTypes::APPLICATION_JSON : header.media_type,
                                           decoded_body);
        if (!success) {
            PropertyCommonReplyHeader reply_header;
            reply_header.status = PropertyExchangeStatus::INTERNAL_ERROR;
            reply_header.message = "Failed to set property: " + header.resource;
            return getReplyHeaderJson(reply_header);
        }
    }

    PropertyCommonReplyHeader reply_header;
    reply_header.status = PropertyExchangeStatus::OK;
    return getReplyHeaderJson(reply_header);
}

// Additional helper methods following Kotlin implementation

std::pair<JsonValue, JsonValue> CommonRulesPropertyService::getPropertyDataJson(const PropertyCommonRequestHeader& header) const {
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
        body = FoundationalResources::toJsonValue(device_.getDeviceInfo());
    } else if (header.resource == PropertyResourceNames::CHANNEL_LIST) {
        body = FoundationalResources::toJsonValue(device_.getConfig().channel_list);
    } else if (header.resource == PropertyResourceNames::JSON_SCHEMA) {
        const auto& json_schema_string = device_.getConfig().json_schema_string;
        if (!json_schema_string.empty()) {
            body = JsonValue::parse(json_schema_string);
        } else {
            body = JsonValue(); // null
        }
    } else {
        auto binary = propertyBinaryGetter(header.resource, header.res_id);
        if (!binary.empty()) {
            std::string body_str(binary.begin(), binary.end());
            body = JsonValue::parse(body_str);
        }
    }
    
    // Property list pagination (Common Rules for PE 6.6.2)
    JsonValue paginated_body = body;
    int total_count = -1;
    
    if (body.isArray() && header.offset.has_value()) {
        auto& array = body.asArray();
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
    
    return std::make_pair(getReplyHeaderJson(reply_header), paginated_body.isNull() ? JsonValue(JsonObject{}) : paginated_body);
}

std::pair<JsonValue, std::vector<uint8_t>> CommonRulesPropertyService::getPropertyDataEncoded(const JsonValue& header_json) const {
    PropertyCommonRequestHeader header = getPropertyHeader(header_json);

    if ((header.media_type.empty() || header.media_type == CommonRulesKnownMimeTypes::APPLICATION_JSON) &&
        (header.mutual_encoding.empty() || header.mutual_encoding == PropertyDataEncoding::ASCII)) {
        auto result = getPropertyDataJson(header);
        std::string body_str = result.second.serialize();
        std::vector<uint8_t> body(body_str.begin(), body_str.end());
        std::vector<uint8_t> encoded_body = helper_->encodeBody(body, header.mutual_encoding);
        return std::make_pair(result.first, encoded_body);
    } else {
        auto body = propertyBinaryGetter(header.resource, header.res_id);
        std::vector<uint8_t> encoded_body = helper_->encodeBody(body, header.mutual_encoding);
        PropertyCommonReplyHeader reply_header;
        reply_header.status = PropertyExchangeStatus::OK;
        reply_header.mutual_encoding = header.mutual_encoding;

        return std::make_pair(getReplyHeaderJson(reply_header), encoded_body);
    }
}

} // namespace
