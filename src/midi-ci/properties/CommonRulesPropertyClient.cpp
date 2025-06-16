#include "midi-ci/properties/CommonRulesPropertyClient.hpp"
#include "midi-ci/properties/PropertyCommonRules.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/json/Json.hpp"

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
    int status = get_header_field_integer(msg.get_header(), PropertyCommonHeaderKeys::STATUS);
    if (status == PropertyExchangeStatus::OK) {
    }
}

void CommonRulesPropertyClient::property_value_updated(const std::string& property_id, const std::vector<uint8_t>& body) {
    if (property_id == PropertyResourceNames::RESOURCE_LIST) {
        for (auto& callback : property_catalog_updated_callbacks_) {
            callback();
        }
    } else if (property_id == PropertyResourceNames::DEVICE_INFO) {
        try {
            std::string body_str(body.begin(), body.end());
            auto json_body = json::JsonValue::parse(body_str);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::CHANNEL_LIST) {
        try {
            std::string body_str(body.begin(), body.end());
            auto json_body = json::JsonValue::parse(body_str);
        } catch (...) {
        }
    } else if (property_id == PropertyResourceNames::JSON_SCHEMA) {
        try {
            std::string body_str(body.begin(), body.end());
            auto json_body = json::JsonValue::parse(body_str);
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
}

void CommonRulesPropertyClient::add_property_catalog_updated_callback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(callback);
}

} // namespace properties
} // namespace midi_ci
