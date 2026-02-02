#include "midicci/midicci.hpp"
#include "midicci/details/commonproperties/StandardProperties.hpp"
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

void PropertyClientFacade::setPropertyRules(std::unique_ptr<MidiCIClientPropertyRules> rules) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_rules_ = std::move(rules);
}

MidiCIClientPropertyRules* PropertyClientFacade::getPropertyRules() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->property_rules_.get();
}

uint8_t PropertyClientFacade::sendGetPropertyData(const std::string& resource, const std::string& res_id, const std::string& encoding, int paginate_offset, int paginate_limit) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = pimpl_->device_.getMessenger().getNextRequestId();

    if (!pimpl_->property_rules_) return request_id;

    std::map<std::string, std::string> fields;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = "false";
    if (paginate_offset >= 0) fields["offset"] = std::to_string(paginate_offset);
    if (paginate_limit >= 0) fields["limit"] = std::to_string(paginate_limit);

    auto header = pimpl_->property_rules_->createDataRequestHeader(resource, fields);
    if (header.empty()) {
        pimpl_->device_.getLogger()(LogData("Failed to create request header for resource: " + resource, true));
        return request_id;
    }

    GetPropertyData msg(
        Common(pimpl_->device_.getMuid(), pimpl_->conn_.getTargetMuid(), 0x7F, 0),
        request_id, header
    );

    sendGetPropertyData(msg);
    return request_id;
}

void PropertyClientFacade::sendGetPropertyData(const GetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->open_requests_[msg.getRequestId()] = msg.serialize(pimpl_->device_.getConfig())[0];
    pimpl_->device_.getMessenger().send(msg);
}

uint8_t PropertyClientFacade::sendSetPropertyData(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, const std::string& encoding, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = pimpl_->device_.getMessenger().getNextRequestId();

    if (!pimpl_->property_rules_) return request_id;

    std::map<std::string, std::string> fields;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!encoding.empty()) fields["mutualEncoding"] = encoding;
    fields["setPartial"] = is_partial ? "true" : "false";

    auto header = pimpl_->property_rules_->createDataRequestHeader(resource, fields);
    auto encoded_body = pimpl_->property_rules_->encodeBody(data, encoding);

    SetPropertyData msg(
        Common(pimpl_->device_.getMuid(), pimpl_->conn_.getTargetMuid(), 0x7F, 0),
        request_id, header, encoded_body
    );

    sendSetPropertyData(msg);
    return request_id;
}

void PropertyClientFacade::sendSetPropertyData(const SetPropertyData& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    pimpl_->open_requests_[msg.getRequestId()] = msg.serialize(pimpl_->device_.getConfig())[0];
    pimpl_->device_.getMessenger().send(msg);
}

void PropertyClientFacade::getPropertyData(const std::string& resource, const std::string& res_id, GetPropertyDataCallback callback, const std::string& encoding, int paginate_offset, int paginate_limit) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = sendGetPropertyData(resource, res_id, encoding, paginate_offset, paginate_limit);
    pimpl_->pending_get_property_callbacks_[request_id] = std::move(callback);
}

void PropertyClientFacade::setPropertyData(const std::string& resource, const std::string& res_id, const std::vector<uint8_t>& data, SetPropertyDataCallback callback, const std::string& encoding, bool is_partial) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto request_id = sendSetPropertyData(resource, res_id, data, encoding, is_partial);
    pimpl_->pending_set_property_callbacks_[request_id] = std::move(callback);
}

void PropertyClientFacade::sendSubscribeProperty(const std::string& resource, const std::string& res_id, const std::string& mutual_encoding, const std::string& subscription_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) return;
    
    std::map<std::string, std::string> fields;
    fields["command"] = MidiCISubscriptionCommand::START;
    if (!res_id.empty()) fields["resId"] = res_id;
    if (!mutual_encoding.empty()) fields["mutualEncoding"] = mutual_encoding;
    
    auto header = pimpl_->property_rules_->createSubscriptionHeader(resource, fields);
    
    auto request_id = pimpl_->device_.getMessenger().getNextRequestId();
    
    SubscribeProperty msg(
        Common(pimpl_->device_.getMuid(), pimpl_->conn_.getTargetMuid(), 0x7F, 0),
        request_id, header, {}
    );
    
    // Create pending subscription entry before sending (like Kotlin implementation)
    addPendingSubscription(request_id, subscription_id, resource, res_id);
    
    pimpl_->device_.getMessenger().send(msg);
}

void PropertyClientFacade::sendUnsubscribeProperty(const std::string& property_id, const std::string& res_id) {
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
    
    auto new_request_id = pimpl_->device_.getMessenger().getNextRequestId();
    
    std::map<std::string, std::string> fields;
    fields["command"] = MidiCISubscriptionCommand::END;
    
    // Include subscription ID if available (critical for host to identify which subscription to end)
    if (sub_it->subscriptionId && !sub_it->subscriptionId->empty()) {
        fields["subscribeId"] = *sub_it->subscriptionId;
    }
    
    auto header = pimpl_->property_rules_->createSubscriptionHeader(property_id, fields);
    
    SubscribeProperty msg(
        Common(pimpl_->device_.getMuid(), pimpl_->conn_.getTargetMuid(), 0x7F, 0),
        new_request_id, header, {}
    );
    
    // Update subscription state to Unsubscribing before sending (like Kotlin implementation)
    promoteSubscriptionAsUnsubscribing(property_id, res_id, new_request_id);
    
    pimpl_->device_.getMessenger().send(msg);
}

void PropertyClientFacade::processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->property_rules_) {
        pimpl_->property_rules_->requestPropertyList(msg.getCommon().group);
    }
}

void PropertyClientFacade::processGetDataReply(const GetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto it = pimpl_->open_requests_.find(msg.getRequestId());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 16) {
            uint32_t source_muid = CIRetrieval::getSourceMuid(data);
            uint32_t dest_muid = CIRetrieval::getDestinationMuid(data);
            uint8_t request_id = data[13];
            std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);

            Common common(source_muid, dest_muid, 0x7F, 0);
            GetPropertyData stored_request(common, request_id, header);

            if (stored_request.getCommon().source_muid == msg.getCommon().destination_muid &&
                stored_request.getCommon().destination_muid == msg.getCommon().source_muid) {

                auto callback_it = pimpl_->pending_get_property_callbacks_.find(msg.getRequestId());
                if (callback_it != pimpl_->pending_get_property_callbacks_.end()) {
                    auto callback = std::move(callback_it->second);
                    pimpl_->pending_get_property_callbacks_.erase(callback_it);
                    callback(msg);
                }

                if (pimpl_->property_rules_) {
                    auto status = pimpl_->property_rules_->getHeaderFieldInteger(msg.getHeader(), "status");
                    if (status == 200) {
                        auto property_id = pimpl_->property_rules_->getPropertyIdForHeader(stored_request.getHeader());
                        auto res_id = pimpl_->property_rules_->getHeaderFieldString(stored_request.getHeader(), "resId");

                        auto media_type = pimpl_->property_rules_->getHeaderFieldString(msg.getHeader(), "mediaType");
                        if (media_type.empty()) {
                            media_type = "application/json";
                        }

                        if (pimpl_->properties_) {
                            pimpl_->properties_->setPropertyValue(property_id, res_id, msg.getBody(), false);
                        }

                        pimpl_->property_rules_->propertyValueUpdated(property_id, msg.getBody());
                        pimpl_->cached_properties_[property_id] = msg.getBody();

                        if (property_id == commonproperties::StandardPropertyNames::ALL_CTRL_LIST) {
                            pimpl_->conn_.setAllCtrlListReceived(true);

                            auto features = pimpl_->conn_.getProcessInquirySupportedFeatures();
                            if (features & static_cast<uint8_t>(MidiCIProcessInquiryFeatures::MIDI_MESSAGE_REPORT)) {
                                pimpl_->device_.getMessenger().sendMidiMessageReportInquiry(
                                    msg.getCommon().group,
                                    msg.getCommon().address,
                                    msg.getSourceMuid(),
                                    static_cast<uint8_t>(MidiMessageReportDataControl::Full),
                                    static_cast<uint8_t>(MidiMessageReportChannelControllerFlags::All),
                                    static_cast<uint8_t>(MidiMessageReportChannelControllerFlags::All),
                                    0
                                );
                            }
                        }
                    }
                }

                pimpl_->open_requests_.erase(it);
            }
        }
    }
}

void PropertyClientFacade::processSetDataReply(const SetPropertyDataReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);

    auto it = pimpl_->open_requests_.find(msg.getRequestId());
    if (it != pimpl_->open_requests_.end()) {
        const auto& data = it->second;
        if (data.size() >= 18) {
            uint32_t source_muid = CIRetrieval::getSourceMuid(data);
            uint32_t dest_muid = CIRetrieval::getDestinationMuid(data);
            uint8_t request_id = data[13];
            std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
            std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);

            Common common(source_muid, dest_muid, 0x7F, 0);
            SetPropertyData stored_request(common, request_id, header, body);

            if (stored_request.getCommon().source_muid == msg.getCommon().destination_muid &&
                stored_request.getCommon().destination_muid == msg.getCommon().source_muid) {

                auto callback_it = pimpl_->pending_set_property_callbacks_.find(msg.getRequestId());
                if (callback_it != pimpl_->pending_set_property_callbacks_.end()) {
                    auto callback = std::move(callback_it->second);
                    pimpl_->pending_set_property_callbacks_.erase(callback_it);
                    callback(msg);
                }

                if (pimpl_->property_rules_) {
                    auto status = pimpl_->property_rules_->getHeaderFieldInteger(msg.getHeader(), "status");
                    if (status == 200) {
                        auto property_id = pimpl_->property_rules_->getPropertyIdForHeader(stored_request.getHeader());
                        pimpl_->property_rules_->propertyValueUpdated(property_id, {});
                    }
                }

                pimpl_->open_requests_.erase(it);
            }
        }
    }
}

SubscribePropertyReply PropertyClientFacade::processSubscribeProperty(const SubscribeProperty& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) {
        // Return error reply if no property rules
        return SubscribePropertyReply(
            Common(pimpl_->device_.getMuid(), msg.getCommon().source_muid, msg.getCommon().address, msg.getCommon().group),
            msg.getRequestId(),
            pimpl_->property_rules_ ? pimpl_->property_rules_->createStatusHeader(PropertyExchangeStatus::INTERNAL_ERROR) : std::vector<uint8_t>{},
            {}
        );
    }
    
    auto command = pimpl_->property_rules_->getHeaderFieldString(msg.getHeader(), "command");
    
    if (command == MidiCISubscriptionCommand::END) {
        return handleUnsubscriptionNotification(msg);
    } else {
        auto result = updatePropertyBySubscribe(msg);
        
        // If the update was NOTIFY, then send Get Data request
        if (result.first == MidiCISubscriptionCommand::NOTIFY) {
            auto property_id = pimpl_->property_rules_->getPropertyIdForHeader(msg.getHeader());
            auto res_id = pimpl_->property_rules_->getResIdForHeader(msg.getHeader());
            sendGetPropertyData(property_id, res_id);
        }
        
        return std::move(result.second);
    }
}

// Helper method: handle unsubscription notification
SubscribePropertyReply PropertyClientFacade::handleUnsubscriptionNotification(const SubscribeProperty& msg) {
    if (!pimpl_->property_rules_) {
        return SubscribePropertyReply(
            Common(pimpl_->device_.getMuid(), msg.getCommon().source_muid, msg.getCommon().address, msg.getCommon().group),
            msg.getRequestId(),
            std::vector<uint8_t>{},
            {}
        );
    }
    
    auto subscription_id = pimpl_->property_rules_->getHeaderFieldString(msg.getHeader(), "subscribeId");
    auto property_id = pimpl_->property_rules_->getPropertyIdForHeader(msg.getHeader());
    
    // Find subscription by subscription ID or property ID
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const ClientSubscription& sub) {
            return (sub.subscriptionId && *sub.subscriptionId == subscription_id) || 
                   sub.propertyId == property_id;
        });
    
    if (it != pimpl_->subscriptions_.end()) {
        // Notify UI before removing the subscription
        notifySubscriptionUpdated(*it);
        pimpl_->subscriptions_.erase(it);
    }
    
    return SubscribePropertyReply(
        Common(pimpl_->device_.getMuid(), msg.getCommon().source_muid, msg.getCommon().address, msg.getCommon().group),
        msg.getRequestId(),
        pimpl_->property_rules_->createStatusHeader(PropertyExchangeStatus::OK),
        {}
    );
}

// Helper method: update property by subscribe
std::pair<std::string, SubscribePropertyReply> PropertyClientFacade::updatePropertyBySubscribe(const SubscribeProperty& msg) {
    if (!pimpl_->property_rules_) {
        return std::make_pair("", SubscribePropertyReply(
            Common(pimpl_->device_.getMuid(), msg.getCommon().source_muid, msg.getCommon().address, msg.getCommon().group),
            msg.getRequestId(),
            std::vector<uint8_t>{},
            {}
        ));
    }
    
    // Use the ObservablePropertyList updateValue method like the Kotlin implementation
    std::string command;
    if (pimpl_->properties_) {
        command = pimpl_->properties_->updateValue(msg);
    } else {
        command = pimpl_->property_rules_->getHeaderFieldString(msg.getHeader(), "command");
    }
    
    // For backwards compatibility, also call propertyValueUpdated if not NOTIFY
    if (command != MidiCISubscriptionCommand::NOTIFY) {
        auto property_id = pimpl_->property_rules_->getSubscribedProperty(msg);
        if (!property_id.empty()) {
            pimpl_->property_rules_->propertyValueUpdated(property_id, msg.getBody());
            pimpl_->cached_properties_[property_id] = msg.getBody();
        }
    }
    
    return std::make_pair(command, SubscribePropertyReply(
        Common(pimpl_->device_.getMuid(), msg.getCommon().source_muid, msg.getCommon().address, msg.getCommon().group),
        msg.getRequestId(),
        pimpl_->property_rules_->createStatusHeader(PropertyExchangeStatus::OK),
        {}
    ));
}

void PropertyClientFacade::processSubscribePropertyReply(const SubscribePropertyReply& msg) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (!pimpl_->property_rules_) {
        return;
    }
    
    // Check if the reply status is OK
    auto status = pimpl_->property_rules_->getHeaderFieldInteger(msg.getHeader(), "status");
    if (status != PropertyExchangeStatus::OK) {
        return; // TODO: should we do anything further here?
    }
    
    auto subscription_id = pimpl_->property_rules_->getHeaderFieldString(msg.getHeader(), "subscribeId");
    
    // Find matching subscription by subscription ID or by pending request ID
    auto sub_it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&](const ClientSubscription& sub) {
            return (sub.subscriptionId && *sub.subscriptionId == subscription_id) ||
                   (sub.pendingRequestId && *sub.pendingRequestId == msg.getRequestId());
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
    pimpl_->property_rules_->processPropertySubscriptionResult(
        const_cast<std::string*>(&sub_it->propertyId), msg);
    
    // Update subscription state
    if (sub_it->state == SubscriptionActionState::Unsubscribing) {
        // Unsubscribe: set state and remove from list
        sub_it->state = SubscriptionActionState::Unsubscribed;
        
        // Notify UI before removing the subscription
        notifySubscriptionUpdated(*sub_it);
        
        pimpl_->subscriptions_.erase(sub_it);
    } else {
        // Subscribe: set state to Subscribed
        sub_it->state = SubscriptionActionState::Subscribed;
        
        // Notify UI of successful subscription
        notifySubscriptionUpdated(*sub_it);
    }
}

void PropertyClientFacade::addPendingSubscription(uint8_t request_id, const std::string& subscription_id, const std::string& property_id, const std::string& res_id) {
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
    notifySubscriptionUpdated(sub);
}

void PropertyClientFacade::promoteSubscriptionAsUnsubscribing(const std::string& property_id, const std::string& res_id, uint8_t new_request_id) {
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
    notifySubscriptionUpdated(*sub_it);
}

std::vector<ClientSubscription> PropertyClientFacade::getSubscriptions() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
}

ClientObservablePropertyList* PropertyClientFacade::getProperties() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->properties_.get();
}

void PropertyClientFacade::addSubscriptionUpdateCallback(SubscriptionUpdateCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->subscription_update_callbacks_.push_back(std::move(callback));
}

void PropertyClientFacade::removeSubscriptionUpdateCallback(const SubscriptionUpdateCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    // Note: Removing std::function by comparison is tricky, typically done by storing ID or using a registry
    // For now, we'll leave this as a placeholder
    // TODO: Implement proper callback removal mechanism if needed
}

PropertyChunkManager& PropertyClientFacade::getPendingChunkManager() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->pending_chunk_manager_;
}

const PropertyChunkManager& PropertyClientFacade::getPendingChunkManager() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->pending_chunk_manager_;
}

void PropertyClientFacade::notifySubscriptionUpdated(const ClientSubscription& subscription) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    for (const auto& callback : pimpl_->subscription_update_callbacks_) {
        callback(subscription);
    }
}

} // namespace
