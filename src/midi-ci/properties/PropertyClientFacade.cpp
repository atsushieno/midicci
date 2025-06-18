#include "midi-ci/properties/PropertyClientFacade.hpp"
#include "midi-ci/properties/PropertyHostFacade.hpp"
#include "midi-ci/properties/CommonRulesPropertyClient.hpp"
#include "midi-ci/properties/CommonRulesPropertyMetadata.hpp"
#include "midi-ci/properties/ObservablePropertyList.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include <mutex>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace midi_ci {
namespace properties {

class PropertyClientFacade::Impl {
public:
    Impl(core::MidiCIDevice& device, core::ClientConnection& conn) : device_(device), conn_(conn), 
          property_rules_(std::make_unique<CommonRulesPropertyClient>(device, conn)) {
        properties_ = std::make_unique<ClientObservablePropertyList>(
            device.get_logger(), property_rules_.get());
    }
    
    core::MidiCIDevice& device_;
    core::ClientConnection& conn_;
    std::unique_ptr<MidiCIClientPropertyRules> property_rules_;
    std::unique_ptr<ClientObservablePropertyList> properties_;
    std::unordered_map<uint8_t, std::vector<uint8_t>> open_requests_;
    std::unordered_map<std::string, std::vector<uint8_t>> cached_properties_;
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<core::ClientSubscription> subscriptions_;
    mutable std::recursive_mutex mutex_;
};

PropertyClientFacade::PropertyClientFacade(core::MidiCIDevice& device, core::ClientConnection& conn) 
    : pimpl_(std::make_unique<Impl>(device, conn)) {}

PropertyClientFacade::~PropertyClientFacade() = default;

void PropertyClientFacade::set_property_rules(std::unique_ptr<MidiCIClientPropertyRules> rules) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_rules_ = std::move(rules);
}

MidiCIClientPropertyRules* PropertyClientFacade::get_property_rules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_rules_.get();
}

void PropertyClientFacade::send_get_property_data(const std::string& resource, const std::string& encoding, int paginate_offset, int paginate_limit) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = "false";
    if (paginate_offset >= 0) fields["offset"] = std::to_string(paginate_offset);
    if (paginate_limit >= 0) fields["limit"] = std::to_string(paginate_limit);
    
    auto header = pimpl_->property_rules_->create_data_request_header(resource, fields);
    
    messages::GetPropertyData msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        pimpl_->device_.get_messenger().get_next_request_id(), header
    );
    
    send_get_property_data(msg);
}

void PropertyClientFacade::send_get_property_data(const messages::GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->open_requests_[msg.get_request_id()] = msg.serialize();
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::send_set_property_data(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, const std::string& encoding, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = is_partial ? "true" : "false";
    
    auto header = pimpl_->property_rules_->create_data_request_header(resource, fields);
    auto encoded_body = pimpl_->property_rules_->encode_body(data, encoding);
    
    messages::SetPropertyData msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        pimpl_->device_.get_messenger().get_next_request_id(), header, encoded_body
    );
    
    send_set_property_data(msg);
}

void PropertyClientFacade::send_set_property_data(const messages::SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->open_requests_[msg.get_request_id()] = msg.serialize();
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::send_subscribe_property(const std::string& resource, const std::string& mutual_encoding, const std::string& subscription_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = "start";
    if (!mutual_encoding.empty()) fields["mutualEncoding"] = mutual_encoding;
    
    auto header = pimpl_->property_rules_->create_subscription_header(resource, fields);
    
    messages::SubscribeProperty msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        pimpl_->device_.get_messenger().get_next_request_id(), header, {}
    );
    
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::send_unsubscribe_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = "end";
    
    auto header = pimpl_->property_rules_->create_subscription_header(property_id, fields);
    
    messages::SubscribeProperty msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        pimpl_->device_.get_messenger().get_next_request_id(), header, {}
    );
    
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::process_property_capabilities_reply(const messages::PropertyGetCapabilitiesReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->request_property_list(msg.get_common().group);
    }
}

void PropertyClientFacade::process_get_data_reply(const messages::GetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = pimpl_->open_requests_.find(msg.get_request_id());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 16) {
            // FIXME: this should reuse our common header parser.
            uint32_t source_muid = data[5] | (data[6] << 8) | (data[7] << 16) | (data[8] << 24);
            uint32_t dest_muid = data[9] | (data[10] << 8) | (data[11] << 16) | (data[12] << 24);
            uint8_t request_id = data[13];
            uint16_t header_size = data[14] | (data[15] << 7);
            std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
            
            messages::Common common(source_muid, dest_muid, 0x7F, 0);
            messages::GetPropertyData stored_request(common, request_id, header);
            
            if (stored_request.get_common().source_muid == msg.get_common().destination_muid &&
                stored_request.get_common().destination_muid == msg.get_common().source_muid) {
                
                if (pimpl_->property_rules_) {
                    auto property_id = pimpl_->property_rules_->get_property_id_for_header(stored_request.get_header());
                    pimpl_->property_rules_->property_value_updated(property_id, msg.get_body());
                    pimpl_->cached_properties_[property_id] = msg.get_body();
                    if (pimpl_->properties_) {
                        pimpl_->properties_->updateValue(property_id, msg.get_body());
                    }
                }
                
                pimpl_->open_requests_.erase(it);
            }
        }
    }
}

void PropertyClientFacade::process_set_data_reply(const messages::SetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto it = pimpl_->open_requests_.find(msg.get_request_id());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 18) {
            uint32_t source_muid = data[5] | (data[6] << 7) | (data[7] << 14) | (data[8] << 21);
            uint32_t dest_muid = data[9] | (data[10] << 7) | (data[11] << 14) | (data[12] << 21);
            uint8_t request_id = data[13];
            uint16_t header_size = data[14] | (data[15] << 7);
            uint16_t body_size = data[16] | (data[17] << 7);
            std::vector<uint8_t> header(data.begin() + 18, data.begin() + 18 + header_size);
            std::vector<uint8_t> body(data.begin() + 18 + header_size, data.begin() + 18 + header_size + body_size);
            
            messages::Common common(source_muid, dest_muid, 0x7F, 0);
            messages::SetPropertyData stored_request(common, request_id, header, body);
            
            if (stored_request.get_common().source_muid == msg.get_common().destination_muid &&
                stored_request.get_common().destination_muid == msg.get_common().source_muid) {
                
                if (pimpl_->property_rules_) {
                    auto status = pimpl_->property_rules_->get_header_field_integer(msg.get_header(), "status");
                    if (status == 200) {
                        auto property_id = pimpl_->property_rules_->get_property_id_for_header(stored_request.get_header());
                        pimpl_->property_rules_->property_value_updated(property_id, {});
                    }
                }
                
                pimpl_->open_requests_.erase(it);
            }
        }
    }
}

void PropertyClientFacade::process_subscribe_property(const messages::SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto command = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "command");
        if (command == "notify") {
            auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.get_header());
            send_get_property_data(property_id);
        }
    }
}

void PropertyClientFacade::process_subscribe_property_reply(const messages::SubscribePropertyReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto status = pimpl_->property_rules_->get_header_field_integer(msg.get_header(), "status");
        if (status == 200) {
            auto subscription_id = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "subscribeId");
        }
    }
}

std::vector<core::ClientSubscription> PropertyClientFacade::get_subscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

ClientObservablePropertyList* PropertyClientFacade::get_properties() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->properties_.get();
}

} // namespace properties
} // namespace midi_ci
