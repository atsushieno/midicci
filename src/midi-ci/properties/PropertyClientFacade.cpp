#include "midi-ci/properties/PropertyClientFacade.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include <mutex>
#include <map>

namespace midi_ci {
namespace properties {

class PropertyClientFacade::Impl {
public:
    Impl(core::MidiCIDevice& device, core::ClientConnection& conn) : device_(device), conn_(conn) {}
    
    core::MidiCIDevice& device_;
    core::ClientConnection& conn_;
    std::unique_ptr<MidiCIClientPropertyRules> property_rules_;
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
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_destination_id(), 0x7F, 0),
        0, header
    );
    
    send_get_property_data(msg);
}

void PropertyClientFacade::send_get_property_data(const messages::GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->conn_.send_message(msg);
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
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_destination_id(), 0x7F, 0),
        0, header, encoded_body
    );
    
    send_set_property_data(msg);
}

void PropertyClientFacade::send_set_property_data(const messages::SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->conn_.send_message(msg);
}

void PropertyClientFacade::send_subscribe_property(const std::string& resource, const std::string& mutual_encoding, const std::string& subscription_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = "start";
    if (!mutual_encoding.empty()) fields["mutualEncoding"] = mutual_encoding;
    
    auto header = pimpl_->property_rules_->create_subscription_header(resource, fields);
    
    messages::SubscribeProperty msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_destination_id(), 0x7F, 0),
        0, header, {}
    );
    
    pimpl_->conn_.send_message(msg);
}

void PropertyClientFacade::send_unsubscribe_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = "end";
    
    auto header = pimpl_->property_rules_->create_subscription_header(property_id, fields);
    
    messages::SubscribeProperty msg(
        messages::Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_destination_id(), 0x7F, 0),
        0, header, {}
    );
    
    pimpl_->conn_.send_message(msg);
}

void PropertyClientFacade::process_property_capabilities_reply(const messages::PropertyGetCapabilitiesReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->request_property_list(msg.common.group);
    }
}

void PropertyClientFacade::process_get_data_reply(const messages::GetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.header);
        pimpl_->property_rules_->property_value_updated(property_id, msg.body);
    }
}

void PropertyClientFacade::process_set_data_reply(const messages::SetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto status = pimpl_->property_rules_->get_header_field_integer(msg.header, "status");
        if (status == 200) {
            auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.header);
            pimpl_->property_rules_->property_value_updated(property_id, {});
        }
    }
}

void PropertyClientFacade::process_subscribe_property(const messages::SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto command = pimpl_->property_rules_->get_header_field_string(msg.header, "command");
        if (command == "notify") {
            auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.header);
            send_get_property_data(property_id);
        }
    }
}

void PropertyClientFacade::process_subscribe_property_reply(const messages::SubscribePropertyReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        auto status = pimpl_->property_rules_->get_header_field_integer(msg.header, "status");
        if (status == 200) {
            auto subscription_id = pimpl_->property_rules_->get_header_field_string(msg.header, "subscribeId");
        }
    }
}

} // namespace properties
} // namespace midi_ci
