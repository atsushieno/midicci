#include "TestPropertyRules.hpp"
#include "midicci/propertycommonrules/CommonRulesPropertyMetadata.hpp"
#include "midicci/Message.hpp"
#include "midicci/PropertyCommonRules.hpp"

TestPropertyRules::TestPropertyRules() {
    initialize_default_properties();
}

void TestPropertyRules::initialize_default_properties() {
    auto deviceInfo = std::make_unique<CommonRulesPropertyMetadata>(PropertyResourceNames::DEVICE_INFO);
    auto channelList = std::make_unique<CommonRulesPropertyMetadata>(PropertyResourceNames::CHANNEL_LIST);
    auto jsonSchema = std::make_unique<CommonRulesPropertyMetadata>(PropertyResourceNames::JSON_SCHEMA);
    
    metadata_list_.push_back(std::move(deviceInfo));
    metadata_list_.push_back(std::move(channelList));
    metadata_list_.push_back(std::move(jsonSchema));
    
    JsonValue deviceInfoValue = JsonValue::empty_object();
    std::string deviceInfoJson = deviceInfoValue.serialize();
    property_values_[PropertyResourceNames::DEVICE_INFO] = std::vector<uint8_t>(deviceInfoJson.begin(), deviceInfoJson.end());
    
    JsonValue channelListValue = JsonValue::empty_array();
    std::string channelListJson = channelListValue.serialize();
    property_values_[PropertyResourceNames::CHANNEL_LIST] = std::vector<uint8_t>(channelListJson.begin(), channelListJson.end());
    
    JsonValue schemaValue = JsonValue::empty_object();
    std::string schemaJson = schemaValue.serialize();
    property_values_[PropertyResourceNames::JSON_SCHEMA] = std::vector<uint8_t>(schemaJson.begin(), schemaJson.end());
}

std::string TestPropertyRules::get_property_id_for_header(const std::vector<uint8_t>& header) {
    return parse_property_id_from_header(header);
}

std::vector<uint8_t> TestPropertyRules::create_update_notification_header(const std::string& property_id, const std::map<std::string, std::string>& fields) {
    return create_json_header(property_id, fields);
}

std::vector<std::unique_ptr<PropertyMetadata>> TestPropertyRules::get_metadata_list() {
    std::vector<std::unique_ptr<PropertyMetadata>> result;
    for (const auto& metadata : metadata_list_) {
        auto copy = std::make_unique<CommonRulesPropertyMetadata>();
        copy->resource = metadata->getPropertyId();
        
        auto it = property_values_.find(metadata->getPropertyId());
        if (it != property_values_.end()) {
            copy->setData(it->second);
        }
        
        result.push_back(std::move(copy));
    }
    return result;
}

GetPropertyDataReply TestPropertyRules::get_property_data(const GetPropertyData& msg) {
    std::string property_id = parse_property_id_from_header(msg.get_header());
    
    auto it = property_values_.find(property_id);
    if (it != property_values_.end()) {
        std::vector<uint8_t> reply_header = create_json_header(property_id);
        return GetPropertyDataReply(msg.get_common(), msg.get_request_id(), reply_header, it->second);
    }
    
    return GetPropertyDataReply(msg.get_common(), msg.get_request_id(), {}, {});
}

SetPropertyDataReply TestPropertyRules::set_property_data(const SetPropertyData& msg) {
    std::string property_id = parse_property_id_from_header(msg.get_header());
    
    property_values_[property_id] = msg.get_body();
    
    std::vector<uint8_t> reply_header = create_json_header(property_id);
    return SetPropertyDataReply(msg.get_common(), msg.get_request_id(), reply_header);
}

SubscribePropertyReply TestPropertyRules::subscribe_property(const SubscribeProperty& msg) {
    std::string property_id = parse_property_id_from_header(msg.get_header());
    
    std::string subscription_id = "sub_" + std::to_string(subscriptions_.size() + 1);
    subscriptions_.emplace_back(msg.get_common().source_muid, property_id, subscription_id, "");
    
    std::vector<uint8_t> reply_header = create_json_header(property_id);
    return SubscribePropertyReply(msg.get_common(), msg.get_request_id(), reply_header, {});
}

void TestPropertyRules::add_metadata(std::unique_ptr<PropertyMetadata> property) {
    auto it = std::find_if(metadata_list_.begin(), metadata_list_.end(),
        [&property](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property->getPropertyId();
        });
    
    if (it != metadata_list_.end()) {
        *it = std::move(property);
    } else {
        metadata_list_.push_back(std::move(property));
    }
}

void TestPropertyRules::remove_metadata(const std::string& property_id) {
    auto it = std::remove_if(metadata_list_.begin(), metadata_list_.end(),
        [&property_id](const std::unique_ptr<PropertyMetadata>& p) {
            return p->getPropertyId() == property_id;
        });
    
    metadata_list_.erase(it, metadata_list_.end());
    property_values_.erase(property_id);
}

std::vector<uint8_t> TestPropertyRules::encode_body(const std::vector<uint8_t>& data, const std::string& encoding) {
    return data;
}

std::vector<uint8_t> TestPropertyRules::decode_body(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    return body;
}

std::string TestPropertyRules::get_header_field_string(const std::vector<uint8_t>& header, const std::string& field) {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.is_string()) {
                return it->second.as_string();
            }
        }
    } catch (...) {
    }
    return "";
}

int TestPropertyRules::get_header_field_integer(const std::vector<uint8_t>& header, const std::string& field) {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(field);
            if (it != obj.end() && it->second.is_number()) {
                return static_cast<int>(it->second.as_number());
            }
        }
    } catch (...) {
    }
    return 200; // Return OK status by default for tests
}

std::vector<uint8_t> TestPropertyRules::create_shutdown_subscription_header(const std::string& property_id) {
    return create_json_header(property_id);
}

const std::vector<SubscriptionEntry>& TestPropertyRules::get_subscriptions() const {
    return subscriptions_;
}

void TestPropertyRules::set_property_value(const std::string& property_id, const std::vector<uint8_t>& data) {
    property_values_[property_id] = data;
}

std::vector<uint8_t> TestPropertyRules::get_property_value(const std::string& property_id) const {
    auto it = property_values_.find(property_id);
    if (it != property_values_.end()) {
        return it->second;
    }
    return {};
}

std::string TestPropertyRules::parse_property_id_from_header(const std::vector<uint8_t>& header) const {
    try {
        std::string header_str(header.begin(), header.end());
        auto json_header = JsonValue::parse(header_str);
        
        if (json_header.is_object()) {
            const auto& obj = json_header.as_object();
            auto it = obj.find(PropertyCommonHeaderKeys::RESOURCE);
            if (it != obj.end() && it->second.is_string()) {
                return it->second.as_string();
            }
        }
    } catch (...) {
    }
    return "";
}

std::vector<uint8_t> TestPropertyRules::create_json_header(const std::string& property_id, const std::map<std::string, std::string>& fields) const {
    JsonObject header_obj;
    header_obj[PropertyCommonHeaderKeys::RESOURCE] = JsonValue(property_id);
    
    for (const auto& [key, value] : fields) {
        if (!value.empty()) {
            header_obj[key] = JsonValue(value);
        }
    }
    
    JsonValue header(header_obj);
    std::string json_str = header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}
