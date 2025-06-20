#include "midicci/messages/Messenger.hpp"
#include "midicci/messages/Message.hpp"
#include "midicci/core/MidiCIDevice.hpp"
#include "midicci/core/MidiCIConstants.hpp"
#include "midicci/core/CIFactory.hpp"
#include "midicci/core/MidiCIDeviceConfiguration.hpp"
#include "midicci/core/ClientConnection.hpp"
#include "midicci/core/CIRetrieval.hpp"
#include "midicci/profiles/ProfileClientFacade.hpp"
#include "midicci/profiles/ProfileHostFacade.hpp"
#include "midicci/properties/PropertyClientFacade.hpp"
#include "midicci/properties/PropertyHostFacade.hpp"
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>
#include <algorithm>
#include <set>

namespace midicci {

class Messenger::Impl {
public:
    explicit Impl(MidiCIDevice& device)
        : device_(device), request_id_counter_(0) {}

    MidiCIDevice& device_;
    std::vector<MessageCallback> callbacks_;
    mutable std::recursive_mutex mutex_;
    std::atomic<uint8_t> request_id_counter_;

    void notify_callbacks(const Message& message) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (const auto& callback : callbacks_) {
            callback(message);
        }
    }

    void log_message(const Message& message, bool is_outgoing) {
        auto logger = device_.get_logger();
        if (logger) {
            logger(message.get_log_message(), is_outgoing);
        }
    }
};

Messenger::Messenger(MidiCIDevice& device)
    : pimpl_(std::make_unique<Impl>(device)) {}

Messenger::~Messenger() = default;

void Messenger::send(const Message& message) {
    pimpl_->log_message(message, true);

    auto parts = message.serialize_multi(pimpl_->device_.get_config());
    for (auto& part : parts) {
        auto ci_output_sender = pimpl_->device_.get_ci_output_sender();
        if (ci_output_sender) {
            uint8_t group = message.get_common().group;
            ci_output_sender(group, part);
        }
        pimpl_->notify_callbacks(message);
    }
}

void Messenger::send_discovery_inquiry(uint8_t ciCategorySupported) {
    Common common(pimpl_->device_.get_muid(), BROADCAST_MUID, MIDI_CI_ADDRESS_FUNCTION_BLOCK, pimpl_->device_.get_config().group);
    auto deviceInfo = pimpl_->device_.get_device_info();
    DeviceDetails details{deviceInfo.manufacturer_id, deviceInfo.family_id, deviceInfo.model_id, deviceInfo.version_id};

    DiscoveryInquiry inquiry(common, details, 0x7F, pimpl_->device_.get_config().receivable_max_sysex_size, false);

    send(inquiry);
}

void Messenger::send_discovery_reply(uint8_t group, uint32_t destination_muid) {
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    auto deviceInfo = pimpl_->device_.get_device_info();
    DeviceDetails details{deviceInfo.manufacturer_id, deviceInfo.family_id, deviceInfo.model_id, deviceInfo.version_id};

    DiscoveryReply reply(common, details, 0x7F, pimpl_->device_.get_config().receivable_max_sysex_size, 0, false);

    send(reply);
}

void Messenger::send_endpoint_inquiry(uint8_t group, uint32_t destination_muid, uint8_t status) {
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    EndpointInquiry inquiry(common, status);
    send(inquiry);
}

void Messenger::send_invalidate_muid(uint8_t group, uint32_t destination_muid, uint32_t target_muid) {
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    InvalidateMUID invalidate(common, target_muid);
    send(invalidate);
}

void Messenger::send_profile_inquiry(uint8_t group, uint32_t destination_muid) {
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    ProfileInquiry inquiry(common);
    send(inquiry);
}

void Messenger::send_set_profile_on(uint8_t group, uint8_t address, uint32_t destination_muid,
                                    const midicci::profilecommonrules::MidiCIProfileId& profile_id, uint16_t num_channels) {
    Common common(pimpl_->device_.get_muid(), destination_muid, address, group);

    SetProfileOn set_on(common, profile_id, num_channels);
    send(set_on);
}

void Messenger::send_set_profile_off(uint8_t group, uint8_t address, uint32_t destination_muid,
                                    const midicci::profilecommonrules::MidiCIProfileId& profile_id) {
    Common common(pimpl_->device_.get_muid(), destination_muid, address, group);

    SetProfileOff set_off(common, profile_id);
    send(set_off);
}

void Messenger::send_profile_enabled_report(uint8_t group, uint8_t address,
                                            const midicci::profilecommonrules::MidiCIProfileId& profile_id, uint16_t num_channels) {

    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileEnabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::send_profile_disabled_report(uint8_t group, uint8_t address,
                                             const midicci::profilecommonrules::MidiCIProfileId& profile_id, uint16_t num_channels) {

    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileDisabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::send_profile_added_report(uint8_t group, uint8_t address,
                                        const midicci::profilecommonrules::MidiCIProfileId& profile_id) {

    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileAddedReport report(common, profile_id);
    send(report);
}

void Messenger::send_profile_removed_report(uint8_t group, uint8_t address,
                                          const midicci::profilecommonrules::MidiCIProfileId& profile_id) {

    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileRemovedReport report(common, profile_id);
    send(report);
}

void Messenger::send_property_get_capabilities(uint8_t group, uint32_t destination_muid,
                                             uint8_t max_simultaneous_requests) {

    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    PropertyGetCapabilities capabilities(common, max_simultaneous_requests);
    send(capabilities);
}

void Messenger::send_property_get_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header) {

    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    GetPropertyData get_data(common, request_id, header);
    send(get_data);
}

void Messenger::send_property_set_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {

    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    SetPropertyData set_data(common, request_id, header, body);
    send(set_data);
}

void Messenger::send_property_subscribe(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                      const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {

    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    SubscribeProperty subscribe(common, request_id, header, body);
    send(subscribe);
}

void Messenger::send_process_inquiry_capabilities(uint8_t group, uint32_t destination_muid) {

    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    ProcessInquiryCapabilities inquiry(common);
    send(inquiry);
}

void Messenger::send_midi_message_report_inquiry(uint8_t group, uint8_t address, uint32_t destination_muid,
                                                uint8_t message_data_control, uint8_t system_messages,
                                                uint8_t channel_controller_messages, uint8_t note_data_messages) {
    Common common(pimpl_->device_.get_muid(), destination_muid, address, group);

    MidiMessageReportInquiry inquiry(common, message_data_control, system_messages,
                                   channel_controller_messages, note_data_messages);
    send(inquiry);
}

void Messenger::process_input(uint8_t group, const std::vector<uint8_t>& data) {

    if (data.size() < 4 ||
        data[0] != MIDI_CI_UNIVERSAL_SYSEX_ID ||
        data[2] != MIDI_CI_SUB_ID_1) {
        return;
    }

    if (data.size() < MIDI_CI_COMMON_HEADER_SIZE) {
        return;
    }

    uint32_t source_muid = CIRetrieval::get_source_muid(data);
    uint32_t dest_muid = CIRetrieval::get_destination_muid(data);
    uint8_t address = CIRetrieval::get_addressing(data);

    Common common(source_muid, dest_muid, address, group);

    if (dest_muid != pimpl_->device_.get_muid() && dest_muid != MIDI_CI_BROADCAST_MUID_32) {
        return;
    }

    CISubId2 message_type = static_cast<CISubId2>(data[3]);

    switch (message_type) {
        case CISubId2::DISCOVERY_REPLY: {
            if (data.size() >= 30) {
                auto device_details = CIRetrieval::get_device_details(data);

                uint8_t ci_supported = data[24];
                uint32_t max_sysex = CIRetrieval::get_max_sysex_size(data);
                uint8_t output_path = data.size() > 29 ? data[29] : 0;
                uint8_t function_block = data.size() > 30 ? data[30] : 0;

                DiscoveryReply reply(common, device_details, ci_supported, max_sysex, output_path, function_block);
                pimpl_->log_message(reply, false);
                processDiscoveryReply(reply);
            }
            break;
        }
        case CISubId2::INVALIDATE_MUID: {
            if (data.size() >= 18) {
                uint32_t target_muid = CIRetrieval::get_muid_to_invalidate(data);
                InvalidateMUID invalidate(common, target_muid);
                pimpl_->log_message(invalidate, false);
                processInvalidateMUID(invalidate);
            }
            break;
        }
        case CISubId2::PROFILE_INQUIRY_REPLY: {
            if (data.size() >= 15) {
                const auto [enabled_profiles, disabled_profiles] = CIRetrieval::get_profile_set(data);
                ProfileReply reply(common, enabled_profiles, disabled_profiles);
                pimpl_->log_message(reply, false);
                processProfileReply(reply);
            }
            break;
        }
        case CISubId2::PROFILE_ADDED_REPORT: {
            if (data.size() >= 18) {
                auto profile_id = CIRetrieval::get_profile_id(data);
                ProfileAdded added(common, profile_id);
                pimpl_->log_message(added, false);
                processProfileAddedReport(added);
            }
            break;
        }
        case CISubId2::PROFILE_REMOVED_REPORT: {
            if (data.size() >= 18) {
                auto profile_id = CIRetrieval::get_profile_id(data);
                ProfileRemoved removed(common, profile_id);
                pimpl_->log_message(removed, false);
                processProfileRemovedReport(removed);
            }
            break;
        }
        case CISubId2::PROFILE_ENABLED_REPORT: {
            if (data.size() >= 20) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                uint16_t channels = CIRetrieval::get_profile_enabled_channels(data);
                ProfileEnabled enabled(common, profile_id, channels);
                pimpl_->log_message(enabled, false);
                processProfileEnabledReport(enabled);
            }
            break;
        }
        case CISubId2::PROFILE_DISABLED_REPORT: {
            if (data.size() >= 20) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                uint16_t channels = CIRetrieval::get_profile_enabled_channels(data);
                ProfileDisabled disabled(common, profile_id, channels);
                pimpl_->log_message(disabled, false);
                processProfileDisabledReport(disabled);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_REPLY: {
            if (data.size() >= 14) {
                uint8_t max_requests = CIRetrieval::get_max_property_requests(data);
                PropertyGetCapabilitiesReply reply(common, max_requests);
                pimpl_->log_message(reply, false);
                processPropertyCapabilitiesReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);
                GetPropertyDataReply reply(common, request_id, header, body);
                pimpl_->log_message(reply, false);
                processGetDataReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                SetPropertyDataReply reply(common, request_id, header);
                pimpl_->log_message(reply, false);
                processSetDataReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);
                SubscribePropertyReply reply(common, request_id, header, body);
                pimpl_->log_message(reply, false);
                processSubscribePropertyReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_NOTIFY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);
                SubscribeProperty notify(common, request_id, header, body);
                pimpl_->log_message(notify, false);
                processPropertyNotify(notify);
            }
            break;
        }
        case CISubId2::DISCOVERY_INQUIRY: {
            if (data.size() >= 30) {
                auto device_details = CIRetrieval::get_device_details(data);

                uint8_t ci_supported = data[24];
                uint32_t max_sysex = CIRetrieval::get_max_sysex_size(data);
                uint8_t output_path = data.size() > 29 ? data[29] : 0;

                DiscoveryInquiry inquiry(common, device_details, ci_supported, max_sysex, output_path);
                pimpl_->log_message(inquiry, false);
                processDiscovery(inquiry);
            }
            break;
        }
        case CISubId2::PROFILE_INQUIRY: {
            ProfileInquiry inquiry(common);
            pimpl_->log_message(inquiry, false);
            processProfileInquiry(inquiry);
            break;
        }
        case CISubId2::PROFILE_SET_ON: {
            if (data.size() >= 20) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                uint16_t channels = CIRetrieval::get_profile_enabled_channels(data);
                SetProfileOn set_on(common, profile_id, channels);
                pimpl_->log_message(set_on, false);
                processSetProfileOn(set_on);
            }
            break;
        }
        case CISubId2::PROFILE_SET_OFF: {
            if (data.size() >= 18) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                SetProfileOff set_off(common, profile_id);
                pimpl_->log_message(set_off, false);
                processSetProfileOff(set_off);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY: {
            if (data.size() >= 14) {
                uint8_t max_requests = CIRetrieval::get_max_property_requests(data);
                PropertyGetCapabilities inquiry(common, max_requests);
                pimpl_->log_message(inquiry, false);
                processPropertyCapabilitiesInquiry(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                GetPropertyData inquiry(common, request_id, header);
                pimpl_->log_message(inquiry, false);
                processGetPropertyData(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);
                SetPropertyData inquiry(common, request_id, header, body);
                pimpl_->log_message(inquiry, false);
                processSetPropertyData(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::get_property_header(data);
                std::vector<uint8_t> body = CIRetrieval::get_property_body_in_this_chunk(data);
                SubscribeProperty inquiry(common, request_id, header, body);
                pimpl_->log_message(inquiry, false);
                processSubscribeProperty(inquiry);
            }
            break;
        }
        case CISubId2::ENDPOINT_MESSAGE_REPLY: {
            if (data.size() >= 16) {
                uint8_t status = data[13];
                uint16_t data_length = data[14] | (data[15] << 7);
                std::vector<uint8_t> endpoint_data(data.begin() + 16, data.begin() + 16 + data_length);
                EndpointReply reply(common, status, endpoint_data);
                pimpl_->log_message(reply, false);
                processEndpointReply(reply);
            }
            break;
        }
        case CISubId2::ACK: {
            if (data.size() >= 23) {
                uint8_t original_sub_id = data[13];
                uint8_t status_code = data[14];
                uint8_t status_data = data[15];
                std::vector<uint8_t> details(data.begin() + 16, data.begin() + 21);
                uint16_t message_length = data[21] | (data[22] << 7);
                std::vector<uint8_t> message_text(data.begin() + 23, data.end());
                processAck(common.source_muid, common.destination_muid, original_sub_id, status_code, status_data, details, message_length, message_text);
            }
            break;
        }
        case CISubId2::NAK: {
            if (data.size() >= 23) {
                uint8_t original_sub_id = data[13];
                uint8_t status_code = data[14];
                uint8_t status_data = data[15];
                std::vector<uint8_t> details(data.begin() + 16, data.begin() + 21);
                uint16_t message_length = data[21] | (data[22] << 7);
                std::vector<uint8_t> message_text(data.begin() + 23, data.end());
                processNak(common.source_muid, common.destination_muid, original_sub_id, status_code, status_data, details, message_length, message_text);
            }
            break;
        }
        case CISubId2::PROFILE_DETAILS_REPLY: {
            if (data.size() >= 22) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                uint8_t target = data[18];
                uint16_t data_size = data[19] | (data[20] << 7);
                std::vector<uint8_t> profile_data(data.begin() + 21, data.begin() + 21 + data_size);
                ProfileDetailsReply reply(common, profile_id, target, profile_data);
                pimpl_->log_message(reply, false);
                processProfileDetailsReply(reply);
            }
            break;
        }
        case CISubId2::PROFILE_SPECIFIC_DATA: {
            if (data.size() >= 22) {
                midicci::profilecommonrules::MidiCIProfileId profile_id = CIRetrieval::get_profile_id(data);
                uint16_t data_length = CIRetrieval::get_profile_specific_data_size(data);
                std::vector<uint8_t> profile_data(data.begin() + 22, data.begin() + 22 + data_length);
                ProfileSpecificData specific_data(common, profile_id, profile_data);
                pimpl_->log_message(specific_data, false);
                processProfileSpecificData(specific_data);
            }
            break;
        }
        case CISubId2::PROCESS_INQUIRY_CAPABILITIES_REPLY: {
            if (data.size() >= 14) {
                uint8_t supported_features = data[13];
                ProcessInquiryCapabilitiesReply reply(common, supported_features);
                pimpl_->log_message(reply, false);
                processProcessInquiryReply(reply);
            }
            break;
        }
        case CISubId2::PROCESS_INQUIRY_MIDI_MESSAGE_REPORT_REPLY: {
            if (data.size() >= 17) {
                uint8_t system_messages = data[13];
                uint8_t channel_controller_messages = data[15];
                uint8_t note_data_messages = data[16];
                MidiMessageReportReply reply(common, system_messages, channel_controller_messages, note_data_messages);
                pimpl_->log_message(reply, false);
                processMidiMessageReportReply(reply);
            }
            break;
        }
        case CISubId2::PROCESS_INQUIRY_END_OF_MIDI_MESSAGE: {
            MidiMessageReportNotifyEnd end_notify(common);
            pimpl_->log_message(end_notify, false);
            processEndOfMidiMessageReport(end_notify);
            break;
        }
        case CISubId2::PROCESS_INQUIRY_MIDI_MESSAGE_REPORT: {
            if (data.size() >= 18) {
                uint8_t message_data_control = data[13];
                uint8_t system_messages = data[14];
                uint8_t channel_controller_messages = data[16];
                uint8_t note_data_messages = data[17];
                MidiMessageReportInquiry inquiry(common, message_data_control, system_messages, channel_controller_messages, note_data_messages);
                pimpl_->log_message(inquiry, false);
                processMidiMessageReport(inquiry);
            }
            break;
        }
        case CISubId2::ENDPOINT_MESSAGE_INQUIRY: {
            if (data.size() >= 14) {
                uint8_t status = data[13];
                EndpointInquiry inquiry(common, status);
                pimpl_->log_message(inquiry, false);
                processEndpointMessage(inquiry);
            }
            break;
        }
        case CISubId2::PROCESS_INQUIRY_CAPABILITIES: {
            ProcessInquiryCapabilities inquiry(common);
            pimpl_->log_message(inquiry, false);
            processProcessInquiry(inquiry);
            break;
        }
        default: {
            processUnknownCIMessage(common, data);
            break;
        }
    }
}

void Messenger::add_message_callback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->callbacks_.push_back(callback);
}

void Messenger::remove_message_callback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->callbacks_.begin(), pimpl_->callbacks_.end(),
        [&callback](const MessageCallback& cb) {
            return cb.target_type() == callback.target_type();
        });
    if (it != pimpl_->callbacks_.end()) {
        pimpl_->callbacks_.erase(it);
    }
}

uint8_t Messenger::get_next_request_id() noexcept {
    return ++pimpl_->request_id_counter_;
}

void Messenger::handleNewEndpoint(const DiscoveryReply& msg) {
    auto existing = pimpl_->device_.get_connection(msg.get_source_muid());
    if (existing) {
        pimpl_->device_.remove_connection(msg.get_source_muid());
    }

    auto connection = std::make_shared<ClientConnection>(pimpl_->device_, msg.get_source_muid(), msg.get_device_details());
    pimpl_->device_.store_connection(msg.get_source_muid(), connection);

    send_endpoint_inquiry(msg.get_common().group, msg.get_source_muid(), 0x01);
    send_profile_inquiry(msg.get_common().group, msg.get_source_muid());
    send_property_get_capabilities(msg.get_common().group, msg.get_source_muid(), 8);
    send_process_inquiry_capabilities(msg.get_common().group, msg.get_source_muid());
}

template<typename MessageType, typename Func>
void Messenger::onClient(const MessageType& msg, Func func) {
    auto connection = pimpl_->device_.get_connection(msg.get_source_muid());
    if (connection) {
        func(connection);
    }
}

EndpointReply Messenger::getEndpointReplyForInquiry(const EndpointInquiry& msg) {
    const auto& config = pimpl_->device_.get_config();
    std::vector<uint8_t> data;

    if (msg.get_status() == 0 && !config.product_instance_id.empty()) {
        const auto& prod_id = config.product_instance_id;
        data.assign(prod_id.begin(), prod_id.end());
    }

    return EndpointReply(
        Common(pimpl_->device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group),
        msg.get_status(),
        data
    );
}

std::vector<ProfileReply> Messenger::getProfileRepliesForInquiry(const ProfileInquiry& msg) {
    std::vector<ProfileReply> replies;
    
    std::vector<uint8_t> addresses;
    if (msg.get_common().address == MIDI_CI_ADDRESS_FUNCTION_BLOCK) {
        auto& profile_host = pimpl_->device_.get_profile_host_facade();
        auto& profiles = profile_host.get_profiles();
        auto all_profiles = profiles.get_profiles();
        
        std::set<uint8_t> unique_addresses;
        for (const auto& profile : all_profiles) {
            unique_addresses.insert(profile.address);
        }
        addresses.assign(unique_addresses.begin(), unique_addresses.end());
    } else {
        addresses.push_back(msg.get_common().address);
    }
    
    for (uint8_t address : addresses) {
        auto& profile_host = pimpl_->device_.get_profile_host_facade();
        auto& profiles = profile_host.get_profiles();
        
        auto enabled_profiles = profiles.get_matching_profiles(address, true);
        auto disabled_profiles = profiles.get_matching_profiles(address, false);
        
        Common common(pimpl_->device_.get_muid(), msg.get_source_muid(), address, msg.get_common().group);
        replies.emplace_back(common, enabled_profiles, disabled_profiles);
    }
    
    return replies;
}

ProcessInquiryCapabilitiesReply Messenger::getProcessInquiryReplyFor(const ProcessInquiryCapabilities& msg) {
    return ProcessInquiryCapabilitiesReply(
        Common(pimpl_->device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group),
        0x01
    );
}

PropertyGetCapabilitiesReply Messenger::getPropertyCapabilitiesReplyFor(const PropertyGetCapabilities& msg) {
    uint8_t max_requests = std::min(msg.get_max_simultaneous_requests(), static_cast<uint8_t>(8));
    return PropertyGetCapabilitiesReply(
        Common(pimpl_->device_.get_muid(), msg.get_source_muid(), msg.get_common().address, msg.get_common().group),
        max_requests
    );
}

void Messenger::processDiscoveryReply(const DiscoveryReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    handleNewEndpoint(msg);
}

void Messenger::processEndpointReply(const EndpointInquiry& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processInvalidateMUID(const InvalidateMUID& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    pimpl_->device_.remove_connection(msg.get_source_muid());
}

void Messenger::processProfileReply(const ProfileReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_reply(msg);
    });
}

void Messenger::processProfileAddedReport(const ProfileAdded& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_added_report(msg);
    });
}

void Messenger::processProfileRemovedReport(const ProfileRemoved& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_removed_report(msg);
    });
}

void Messenger::processProfileEnabledReport(const ProfileEnabled& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_enabled_report(msg);
    });
}

void Messenger::processProfileDisabledReport(const ProfileDisabled& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_disabled_report(msg);
    });
}

void Messenger::processProfileDetailsReply(const ProfileDetailsReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_profile_client_facade().process_profile_details_reply(msg);
    });
}

void Messenger::processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_property_client_facade().process_property_capabilities_reply(msg);
    });
}

void Messenger::processGetDataReply(const GetPropertyDataReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_property_client_facade().process_get_data_reply(msg);
    });
}

void Messenger::processSetDataReply(const SetPropertyDataReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_property_client_facade().process_set_data_reply(msg);
    });
}

void Messenger::processSubscribePropertyReply(const SubscribePropertyReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->get_property_client_facade().process_subscribe_property_reply(msg);
    });
}

void Messenger::processPropertyNotify(const SubscribeProperty& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processProcessInquiryReply(const ProcessInquiryCapabilitiesReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processDiscovery(const DiscoveryInquiry& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    send_discovery_reply(msg.get_common().group, msg.get_source_muid());
}

void Messenger::processEndpointMessage(const EndpointInquiry& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    send(getEndpointReplyForInquiry(msg));
}

void Messenger::processProfileInquiry(const ProfileInquiry& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    auto replies = getProfileRepliesForInquiry(msg);
    for (const auto& reply : replies) {
        send(reply);
    }
}

void Messenger::processSetProfileOn(const SetProfileOn& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processSetProfileOff(const SetProfileOff& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processProfileDetailsInquiry(const ProfileDetailsReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processPropertyCapabilitiesInquiry(const PropertyGetCapabilities& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    send(getPropertyCapabilitiesReplyFor(msg));
}

void Messenger::processGetPropertyData(const GetPropertyData& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    auto reply = pimpl_->device_.get_property_host_facade().process_get_property_data(msg);
    send(reply);
}

void Messenger::processSetPropertyData(const SetPropertyData& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    auto reply = pimpl_->device_.get_property_host_facade().process_set_property_data(msg);
    send(reply);
}

void Messenger::processSubscribeProperty(const SubscribeProperty& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    auto reply = pimpl_->device_.get_property_host_facade().process_subscribe_property(msg);
    send(reply);
}

void Messenger::processProcessInquiry(const ProcessInquiryCapabilities& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
    send(getProcessInquiryReplyFor(msg));
}

void Messenger::processUnknownCIMessage(const Common& common, const std::vector<uint8_t>& data) {
}

void Messenger::processMidiMessageReport(const MidiMessageReportInquiry& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processMidiMessageReportReply(const MidiMessageReportReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processEndOfMidiMessageReport(const MidiMessageReportNotifyEnd& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processEndpointReply(const EndpointReply& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

void Messenger::processAck(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text) {
}

void Messenger::processNak(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text) {
}

void Messenger::processProfileSpecificData(const ProfileSpecificData& msg) {
    for (const auto& callback : pimpl_->callbacks_) {
        callback(msg);
    }
}

} // namespace midi_ci
