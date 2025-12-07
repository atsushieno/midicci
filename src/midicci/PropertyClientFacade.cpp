#include "midicci/midicci.hpp"
#include <mutex>
#include <map>
#include <unordered_map>
#include <algorithm>

namespace midicci {

class PropertyClientFacade::Impl {
public:
    Impl(MidiCIDevice& device, ClientConnection& conn) : device_(device), conn_(conn),
          property_rules_(std::make_unique<CommonRulesPropertyClient>(device, conn)) {
        properties_ = std::make_unique<ClientObservablePropertyList>(property_rules_.get());
    }

    MidiCIDevice& device_;
    ClientConnection& conn_;
    std::unique_ptr<MidiCIClientPropertyRules> property_rules_;
    std::unique_ptr<ClientObservablePropertyList> properties_;
    std::unordered_map<uint8_t, std::vector<uint8_t>> open_requests_;
    std::unordered_map<std::string, std::vector<uint8_t>> cached_properties_;
    std::vector<std::unique_ptr<PropertyMetadata>> metadata_list_;
    std::vector<ClientSubscription> subscriptions_;
    std::vector<PropertyClientFacade::SubscriptionUpdateCallback> subscription_update_callbacks_;
    std::unordered_map<uint8_t, PropertyClientFacade::GetPropertyDataCallback> pending_get_property_callbacks_;
    std::unordered_map<uint8_t, PropertyClientFacade::SetPropertyDataCallback> pending_set_property_callbacks_;
    PropertyChunkManager pending_chunk_manager_;
    mutable std::recursive_mutex mutex_;
};

PropertyClientFacade::PropertyClientFacade(MidiCIDevice& device, ClientConnection& conn) 
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

uint8_t PropertyClientFacade::send_get_property_data(const std::string& resource, const std::string& res_id, const std::string& encoding, int paginate_offset, int paginate_limit) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = pimpl_->device_.get_messenger().get_next_request_id();

    if (!pimpl_->property_rules_) return request_id;

    std::map<std::string, std::string> fields;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = "false";
    if (paginate_offset >= 0) fields["offset"] = std::to_string(paginate_offset);
    if (paginate_limit >= 0) fields["limit"] = std::to_string(paginate_limit);

    auto header = pimpl_->property_rules_->create_data_request_header(resource, fields);
    if (header.empty()) {
        pimpl_->device_.get_logger()(LogData("Failed to create request header for resource: " + resource, true));
        return request_id;
    }

    GetPropertyData msg(
        Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        request_id, header
    );

    send_get_property_data(msg);
    return request_id;
}

void PropertyClientFacade::send_get_property_data(const GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->open_requests_[msg.get_request_id()] = msg.serialize(pimpl_->device_.get_config())[0];
    pimpl_->device_.get_messenger().send(msg);
}

uint8_t PropertyClientFacade::send_set_property_data(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, const std::string& encoding, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = pimpl_->device_.get_messenger().get_next_request_id();

    if (!pimpl_->property_rules_) return request_id;

    std::map<std::string, std::string> fields;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = is_partial ? "true" : "false";

    auto header = pimpl_->property_rules_->create_data_request_header(resource, fields);
    auto encoded_body = pimpl_->property_rules_->encode_body(data, encoding);

    SetPropertyData msg(
        Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        request_id, header, encoded_body
    );

    send_set_property_data(msg);
    return request_id;
}

void PropertyClientFacade::send_set_property_data(const SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    pimpl_->open_requests_[msg.get_request_id()] = msg.serialize(pimpl_->device_.get_config())[0];
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::get_property_data(const std::string& resource, const std::string& res_id, GetPropertyDataCallback callback, const std::string& encoding, int paginate_offset, int paginate_limit) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = send_get_property_data(resource, res_id, encoding, paginate_offset, paginate_limit);
    pimpl_->pending_get_property_callbacks_[request_id] = std::move(callback);
}

void PropertyClientFacade::set_property_data(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, SetPropertyDataCallback callback, const std::string& encoding, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = send_set_property_data(resource, res_id, data, encoding, is_partial);
    pimpl_->pending_set_property_callbacks_[request_id] = std::move(callback);
}

void PropertyClientFacade::send_subscribe_property(const std::string& resource, const std::string& res_id, const std::string& mutual_encoding, const std::string& subscription_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = MidiCISubscriptionCommand::START;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!mutual_encoding.empty()) fields["mutualEncoding"] = mutual_encoding;
    
    auto header = pimpl_->property_rules_->create_subscription_header(resource, fields);
    
    auto request_id = pimpl_->device_.get_messenger().get_next_request_id();
    
    SubscribeProperty msg(
        Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        request_id, header, {}
    );
    
    // Create pending subscription entry before sending (like Kotlin implementation)
    add_pending_subscription(request_id, subscription_id, resource, res_id);
    
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::send_unsubscribe_property(const std::string& property_id, const std::string& res_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    // Find existing subscription for this property and resId
    auto sub_it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const ClientSubscription& sub) {
            return sub.propertyId == property_id && (res_id.empty() || sub.resId == res_id);
        });
    
    if (sub_it == pimpl_->subscriptions_.end()) {
        return;
    }
    
    if (sub_it->state == SubscriptionActionState::Unsubscribing) {
        return;
    }
    
    auto new_request_id = pimpl_->device_.get_messenger().get_next_request_id();
    
    std::map<std::string, std::string> fields;
    fields["command"] = MidiCISubscriptionCommand::END;
    
    // Include subscription ID if available (critical for host to identify which subscription to end)
    if (sub_it->subscriptionId && !sub_it->subscriptionId->empty()) {
        fields["subscribeId"] = *sub_it->subscriptionId;
    }
    
    auto header = pimpl_->property_rules_->create_subscription_header(property_id, fields);
    
    SubscribeProperty msg(
        Common(pimpl_->device_.get_muid(), pimpl_->conn_.get_target_muid(), 0x7F, 0),
        new_request_id, header, {}
    );
    
    // Update subscription state to Unsubscribing before sending (like Kotlin implementation)
    promote_subscription_as_unsubscribing(property_id, res_id, new_request_id);
    
    pimpl_->device_.get_messenger().send(msg);
}

void PropertyClientFacade::process_property_capabilities_reply(const PropertyGetCapabilitiesReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->request_property_list(msg.get_common().group);
    }
}

void PropertyClientFacade::process_get_data_reply(const GetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto it = pimpl_->open_requests_.find(msg.get_request_id());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 16) {
            uint32_t source_muid = CIRetrieval::get_source_muid(data);
            uint32_t dest_muid = CIRetrieval::get_destination_muid(data);
            uint8_t request_id = data[13];
            std::vector<uint8_t> header = CIRetrieval::get_property_header(data);

            Common common(source_muid, dest_muid, 0x7F, 0);
            GetPropertyData stored_request(common, request_id, header);

            if (stored_request.get_common().source_muid == msg.get_common().destination_muid &&
                stored_request.get_common().destination_muid == msg.get_common().source_muid) {

                auto callback_it = pimpl_->pending_get_property_callbacks_.find(msg.get_request_id());
                if (callback_it != pimpl_->pending_get_property_callbacks_.end()) {
                    auto callback = std::move(callback_it->second);
                    pimpl_->pending_get_property_callbacks_.erase(callback_it);
                    callback(msg);
                }

                if (pimpl_->property_rules_) {
                    auto status = pimpl_->property_rules_->get_header_field_integer(msg.get_header(), "status");
                    if (status == 200) {
                        auto property_id = pimpl_->property_rules_->get_property_id_for_header(stored_request.get_header());

                        auto media_type = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "mediaType");
                        if (media_type.empty()) {
                            media_type = "application/json";
                        }

                        if (pimpl_->properties_) {
                            pimpl_->properties_->updateValue(property_id, msg.get_body(), media_type);
                        }

                        pimpl_->property_rules_->property_value_updated(property_id, msg.get_body());
                        pimpl_->cached_properties_[property_id] = msg.get_body();
                    }
                }

                pimpl_->open_requests_.erase(it);
            }
        }
    }
}

void PropertyClientFacade::process_set_data_reply(const SetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto it = pimpl_->open_requests_.find(msg.get_request_id());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 18) {
            uint32_t source_muid = CIRetrieval::get_source_muid(data);
            uint32_t dest_muid = CIRetrieval::get_destination_muid(data);
            uint8_t request_id = data[13];
            std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
            std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);

            Common common(source_muid, dest_muid, 0x7F, 0);
            SetPropertyData stored_request(common, request_id, header, body);

            if (stored_request.get_common().source_muid == msg.get_common().destination_muid &&
                stored_request.get_common().destination_muid == msg.get_common().source_muid) {

                auto callback_it = pimpl_->pending_set_property_callbacks_.find(msg.get_request_id());
                if (callback_it != pimpl_->pending_set_property_callbacks_.end()) {
                    auto callback = std::move(callback_it->second);
                    pimpl_->pending_set_property_callbacks_.erase(callback_it);
                    callback(msg);
                }

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

SubscribePropertyReply PropertyClientFacade::process_subscribe_property(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) {
        // Return error reply if no property rules
        return SubscribePropertyReply(
            Common(pimpl_->device_.get_muid(), msg.get_common().source_muid, msg.get_common().address, msg.get_common().group),
            msg.get_request_id(),
            pimpl_->property_rules_ ? pimpl_->property_rules_->create_status_header(PropertyExchangeStatus::INTERNAL_ERROR) : std::vector<uint8_t>{},
            {}
        );
    }
    
    auto command = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "command");
    
    if (command == MidiCISubscriptionCommand::END) {
        return handle_unsubscription_notification(msg);
    } else {
        auto result = update_property_by_subscribe(msg);
        
        // If the update was NOTIFY, then send Get Data request
        if (result.first == MidiCISubscriptionCommand::NOTIFY) {
            auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.get_header());
            auto res_id = pimpl_->property_rules_->get_res_id_for_header(msg.get_header());
            send_get_property_data(property_id, res_id);
        }
        
        return std::move(result.second);
    }
}

// Helper method: handle unsubscription notification
SubscribePropertyReply PropertyClientFacade::handle_unsubscription_notification(const SubscribeProperty& msg) {
    if (!pimpl_->property_rules_) {
        return SubscribePropertyReply(
            Common(pimpl_->device_.get_muid(), msg.get_common().source_muid, msg.get_common().address, msg.get_common().group),
            msg.get_request_id(),
            std::vector<uint8_t>{},
            {}
        );
    }
    
    auto subscription_id = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "subscribeId");
    auto property_id = pimpl_->property_rules_->get_property_id_for_header(msg.get_header());
    
    // Find subscription by subscription ID or property ID
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const ClientSubscription& sub) {
            return (sub.subscriptionId && *sub.subscriptionId == subscription_id) || 
                   sub.propertyId == property_id;
        });
    
    if (it != pimpl_->subscriptions_.end()) {
        // Notify UI before removing the subscription
        notify_subscription_updated(*it);
        pimpl_->subscriptions_.erase(it);
    }
    
    return SubscribePropertyReply(
        Common(pimpl_->device_.get_muid(), msg.get_common().source_muid, msg.get_common().address, msg.get_common().group),
        msg.get_request_id(),
        pimpl_->property_rules_->create_status_header(PropertyExchangeStatus::OK),
        {}
    );
}

// Helper method: update property by subscribe
std::pair<std::string, SubscribePropertyReply> PropertyClientFacade::update_property_by_subscribe(const SubscribeProperty& msg) {
    if (!pimpl_->property_rules_) {
        return std::make_pair("", SubscribePropertyReply(
            Common(pimpl_->device_.get_muid(), msg.get_common().source_muid, msg.get_common().address, msg.get_common().group),
            msg.get_request_id(),
            std::vector<uint8_t>{},
            {}
        ));
    }
    
    // Use the ObservablePropertyList updateValue method like the Kotlin implementation
    std::string command;
    if (pimpl_->properties_) {
        command = pimpl_->properties_->updateValue(msg);
    } else {
        command = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "command");
    }
    
    // For backwards compatibility, also call property_value_updated if not NOTIFY
    if (command != MidiCISubscriptionCommand::NOTIFY) {
        auto property_id = pimpl_->property_rules_->get_subscribed_property(msg);
        if (!property_id.empty()) {
            pimpl_->property_rules_->property_value_updated(property_id, msg.get_body());
            pimpl_->cached_properties_[property_id] = msg.get_body();
        }
    }
    
    return std::make_pair(command, SubscribePropertyReply(
        Common(pimpl_->device_.get_muid(), msg.get_common().source_muid, msg.get_common().address, msg.get_common().group),
        msg.get_request_id(),
        pimpl_->property_rules_->create_status_header(PropertyExchangeStatus::OK),
        {}
    ));
}

void PropertyClientFacade::process_subscribe_property_reply(const SubscribePropertyReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) {
        return;
    }
    
    // Check if the reply status is OK
    auto status = pimpl_->property_rules_->get_header_field_integer(msg.get_header(), "status");
    if (status != PropertyExchangeStatus::OK) {
        return; // TODO: should we do anything further here?
    }
    
    auto subscription_id = pimpl_->property_rules_->get_header_field_string(msg.get_header(), "subscribeId");
    
    // Find matching subscription by subscription ID or by pending request ID
    auto sub_it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const ClientSubscription& sub) {
            return (sub.subscriptionId && *sub.subscriptionId == subscription_id) ||
                   (sub.pendingRequestId && *sub.pendingRequestId == msg.get_request_id());
        });
    
    if (sub_it == pimpl_->subscriptions_.end()) {
        return;
    }
    
    // Validate subscription ID is present (except for unsubscription reply)
    if (subscription_id.empty() && sub_it->state != SubscriptionActionState::Unsubscribing) {
        return;
    }
    
    // Check subscription state is valid
    if (sub_it->state == SubscriptionActionState::Subscribed || 
        sub_it->state == SubscriptionActionState::Unsubscribed) {
        return;
    }
    
    // Update subscription ID
    if (!subscription_id.empty()) {
        sub_it->subscriptionId = subscription_id;
    }
    
    // Delegate to property rules for rule-specific processing
    pimpl_->property_rules_->process_property_subscription_result(
        const_cast<std::string*>(&sub_it->propertyId), msg);
    
    // Update subscription state
    if (sub_it->state == SubscriptionActionState::Unsubscribing) {
        // Unsubscribe: set state and remove from list
        sub_it->state = SubscriptionActionState::Unsubscribed;
        
        // Notify UI before removing the subscription
        notify_subscription_updated(*sub_it);
        
        pimpl_->subscriptions_.erase(sub_it);
    } else {
        // Subscribe: set state to Subscribed
        sub_it->state = SubscriptionActionState::Subscribed;
        
        // Notify UI of successful subscription
        notify_subscription_updated(*sub_it);
    }
}

void PropertyClientFacade::add_pending_subscription(uint8_t request_id, const std::string& subscription_id, const std::string& property_id, const std::string& res_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    ClientSubscription sub;
    sub.pendingRequestId = request_id;
    if (!subscription_id.empty()) {
        sub.subscriptionId = subscription_id;
    }
    sub.propertyId = property_id;
    sub.resId = res_id;
    sub.state = SubscriptionActionState::Subscribing;
    
    pimpl_->subscriptions_.push_back(sub);
    
    // Notify UI of subscription state change
    notify_subscription_updated(sub);
}

void PropertyClientFacade::promote_subscription_as_unsubscribing(const std::string& property_id, const std::string& res_id, uint8_t new_request_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    auto sub_it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](ClientSubscription& sub) {
            return sub.propertyId == property_id && (res_id.empty() || sub.resId == res_id);
        });
    
    if (sub_it == pimpl_->subscriptions_.end()) {
        return;
    }
    
    if (sub_it->state == SubscriptionActionState::Unsubscribing) {
        return;
    }
    
    sub_it->pendingRequestId = new_request_id;
    sub_it->state = SubscriptionActionState::Unsubscribing;
    
    // Notify UI of subscription state change
    notify_subscription_updated(*sub_it);
}

std::vector<ClientSubscription> PropertyClientFacade::get_subscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

ClientObservablePropertyList* PropertyClientFacade::get_properties() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->properties_.get();
}

void PropertyClientFacade::add_subscription_update_callback(SubscriptionUpdateCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->subscription_update_callbacks_.push_back(std::move(callback));
}

void PropertyClientFacade::remove_subscription_update_callback(const SubscriptionUpdateCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    // Note: Removing std::function by comparison is tricky, typically done by storing ID or using a registry
    // For now, we'll leave this as a placeholder
    // TODO: Implement proper callback removal mechanism if needed
}

PropertyChunkManager& PropertyClientFacade::get_pending_chunk_manager() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->pending_chunk_manager_;
}

const PropertyChunkManager& PropertyClientFacade::get_pending_chunk_manager() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->pending_chunk_manager_;
}

void PropertyClientFacade::notify_subscription_updated(const ClientSubscription& subscription) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    for (const auto& callback : pimpl_->subscription_update_callbacks_) {
        callback(subscription);
    }
}

} // namespace
