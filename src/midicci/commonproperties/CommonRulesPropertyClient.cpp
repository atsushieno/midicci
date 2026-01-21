#include "midicci/midicci.hpp"
#include <midicci/details/commonproperties/StandardProperties.hpp>
#include <sstream>
#include <algorithm>

namespace midicci {
namespace commonproperties {

CommonRulesPropertyClient::CommonRulesPropertyClient(MidiCIDevice& device, ClientConnection& conn)
    : device_(device), conn_(conn), helper_(std::make_unique<CommonRulesPropertyHelper>(device)) {}

std::vector<uint8_t> CommonRulesPropertyClient::createDataRequestHeader(
    const std::string& resource, const std::map<std::string, std::string>& fields) {
    return helper_->createRequestHeaderBytes(resource, fields);
}

std::vector<uint8_t> CommonRulesPropertyClient::createSubscriptionHeader(
    const std::string& resource, const std::map<std::string, std::string>& fields) {
    
    auto command_it = fields.find(PropertyCommonHeaderKeys::COMMAND);
    auto encoding_it = fields.find(PropertyCommonHeaderKeys::MUTUAL_ENCODING);
    
    std::string command = (command_it != fields.end()) ? command_it->second : "";
    std::string encoding = (encoding_it != fields.end()) ? encoding_it->second : "";
    
    return helper_->createSubscribeHeaderBytes(resource, command, encoding);
}

std::vector<uint8_t> CommonRulesPropertyClient::createStatusHeader(int status) {
    JsonObject status_obj;
    status_obj[PropertyCommonHeaderKeys::STATUS] = JsonValue(status);
    
    JsonValue status_header(status_obj);
    auto json_str = status_header.serialize();
    return std::vector<uint8_t>(json_str.begin(), json_str.end());
}

std::vector<uint8_t> CommonRulesPropertyClient::encodeBody(const std::vector<uint8_t>& data, const std::string& encoding) {
    return helper_->encodeBody(data, encoding);
}

std::vector<uint8_t> CommonRulesPropertyClient::decodeBody(const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    return helper_->decodeBody(header, body);
}

std::string CommonRulesPropertyClient::getPropertyIdForHeader(const std::vector<uint8_t>& header) {
    return helper_->getPropertyIdentifierInternal(header);
}

std::string CommonRulesPropertyClient::getResIdForHeader(const std::vector<uint8_t>& header) {
    return helper_->getHeaderFieldString(header, PropertyCommonHeaderKeys::RES_ID);
}

std::string CommonRulesPropertyClient::getHeaderFieldString(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->getHeaderFieldString(header, field);
}

int CommonRulesPropertyClient::getHeaderFieldInteger(const std::vector<uint8_t>& header, const std::string& field) {
    return helper_->getHeaderFieldInteger(header, field);
}

void CommonRulesPropertyClient::processPropertySubscriptionResult(void* sub, const SubscribePropertyReply& msg) {
    if (!sub) return;
    
    int status = getHeaderFieldInteger(msg.getHeader(), PropertyCommonHeaderKeys::STATUS);
    if (status != PropertyExchangeStatus::OK) {
        return;
    }
    
    std::string subscribe_id = getHeaderFieldString(msg.getHeader(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
    if (subscribe_id.empty()) {
        return;
    }
    
    std::string* property_id_ptr = static_cast<std::string*>(sub);
    std::string property_id = *property_id_ptr;
    std::string res_id = getHeaderFieldString(msg.getHeader(), PropertyCommonHeaderKeys::RES_ID);
    
    auto it = std::find_if(subscriptions_.begin(), subscriptions_.end(),
        [&](const SubscriptionEntry& s) { return s.resource == property_id && (res_id.empty() || s.res_id == res_id); });
    
    if (it != subscriptions_.end()) {
        subscriptions_.erase(it);
    }
    
    subscriptions_.emplace_back(conn_.getTargetMuid(), property_id, res_id, subscribe_id, "");
}

void CommonRulesPropertyClient::propertyValueUpdated(const std::string& property_id, const std::vector<uint8_t>& body) {
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
                    conn_.getPropertyClientFacade().sendGetPropertyData(PropertyResourceNames::DEVICE_INFO, "");
                }
            }

            auto allCtrlIt = std::find_if(resource_list_.begin(), resource_list_.end(),
                [](const std::unique_ptr<PropertyMetadata>& p) { return p->getPropertyId() == StandardPropertyNames::ALL_CTRL_LIST; });
            if (allCtrlIt != resource_list_.end()) {
                conn_.getPropertyClientFacade().sendGetPropertyData(StandardPropertyNames::ALL_CTRL_LIST, "");
            }

            auto programIt = std::find_if(resource_list_.begin(), resource_list_.end(),
                [](const std::unique_ptr<PropertyMetadata>& p) { return p->getPropertyId() == StandardPropertyNames::PROGRAM_LIST; });
            if (programIt != resource_list_.end()) {
                conn_.getPropertyClientFacade().sendGetPropertyData(StandardPropertyNames::PROGRAM_LIST, "");
            }
        } catch (const std::exception& ex) {
            device_.getLogger()(LogData("Error parsing resource list: " + std::string(ex.what()), true));
        }
    }
    // Note: DeviceInfo, ChannelList, and JsonSchema are now handled via FoundationalResources
    // and should be accessed through ObservablePropertyList extension methods
}

void CommonRulesPropertyClient::requestPropertyList(uint8_t group) {
    auto request_bytes = helper_->getResourceListRequestBytes();
    
    GetPropertyData msg(
        Common(device_.getMuid(), conn_.getTargetMuid(), 0x7F, group),
        0,
        request_bytes
    );
    
    conn_.getPropertyClientFacade().sendGetPropertyData(msg);
}

std::string CommonRulesPropertyClient::getSubscribedProperty(const SubscribeProperty& msg) {
    auto subscribe_id = getHeaderFieldString(msg.getHeader(), PropertyCommonHeaderKeys::SUBSCRIBE_ID);
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

void CommonRulesPropertyClient::addPropertyCatalogUpdatedCallback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(callback);
}

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyClient::getMetadataList() const {
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

std::vector<std::unique_ptr<PropertyMetadata>> CommonRulesPropertyClient::getMetadataListForBody(const std::vector<uint8_t>& body) {
    try {
        return FoundationalResources::parseResourceList(body);
    } catch (const std::exception& ex) {
        device_.getLogger()(LogData("Error parsing metadata list: " + std::string(ex.what()), true));
        return std::vector<std::unique_ptr<PropertyMetadata>>();
    }
}


} // namespace properties
} // namespace midi_ci
