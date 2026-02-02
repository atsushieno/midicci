#include "midicci/midicci.hpp"
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>
#include <algorithm>
#include <set>
#include <chrono>
#include <thread>
#include <iostream>

namespace midicci {

class Messenger::Impl {
public:
    explicit Impl(MidiCIDevice& device)
        : device_(device), request_id_counter_(0), local_chunk_manager_() {}

    MidiCIDevice& device_;
    std::vector<MessageCallback> callbacks_;
    mutable std::recursive_mutex mutex_;
    std::atomic<uint8_t> request_id_counter_;
    PropertyChunkManager local_chunk_manager_;

    void notify_callbacks(const Message& message) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (const auto& callback : callbacks_) {
            callback(message);
        }
    }

    void log_message(const Message& message, bool is_outgoing) {
        auto logger = device_.getLogger();
        if (logger) {
            logger(LogData(message, is_outgoing));
        }
    }
};

Messenger::Messenger(MidiCIDevice& device)
    : pimpl_(std::make_unique<Impl>(device)) {}

Messenger::~Messenger() = default;

void Messenger::send(const Message& message) {
    pimpl_->log_message(message, true);

    auto& device_config = pimpl_->device_.getConfig();
    int original_chunk_size = device_config.max_property_chunk_size;
    int original_sysex_size = device_config.receivable_max_sysex_size;
    bool overridden = false;
    uint32_t dest_muid = message.getDestinationMuid();
    if (dest_muid != MIDI_CI_BROADCAST_MUID_32) {
        auto connection = pimpl_->device_.getConnection(dest_muid);
        if (connection) {
            uint32_t remote_limit = connection->getRemoteMaxSysexSize();
            if (remote_limit > 0) {
                int limit = static_cast<int>(remote_limit);
                if (device_config.max_property_chunk_size > limit) {
                    device_config.max_property_chunk_size = limit;
                    overridden = true;
                }
                if (device_config.receivable_max_sysex_size > limit) {
                    device_config.receivable_max_sysex_size = limit;
                    overridden = true;
                }
            }
        }
    }

    auto parts = message.serializeMulti(device_config);
    if (overridden) {
        device_config.max_property_chunk_size = original_chunk_size;
        device_config.receivable_max_sysex_size = original_sysex_size;
    }
    for (auto & part : parts) {
        auto ci_output_sender = pimpl_->device_.getCiOutputSender();
        if (ci_output_sender) {
            uint8_t group = message.getCommon().group;
            if (!ci_output_sender(group, part))
                throw std::runtime_error("Failed to send MIDI-CI message");
        }
    }
}

void Messenger::sendDiscoveryInquiry(uint8_t ciCategorySupported) {
    uint32_t our_muid = pimpl_->device_.getMuid();
    Common common(our_muid, BROADCAST_MUID, MIDI_CI_ADDRESS_FUNCTION_BLOCK, pimpl_->device_.getConfig().group);
    auto deviceInfo = pimpl_->device_.getDeviceInfo();
    DeviceDetails details{deviceInfo.manufacturer_id, deviceInfo.family_id, deviceInfo.model_id, deviceInfo.version_id};

    DiscoveryInquiry inquiry(common, details, 0x7F, pimpl_->device_.getConfig().receivable_max_sysex_size, false);

    send(inquiry);
}

void Messenger::sendDiscoveryReply(uint8_t group, uint32_t destination_muid) {
    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    auto deviceInfo = pimpl_->device_.getDeviceInfo();
    DeviceDetails details{deviceInfo.manufacturer_id, deviceInfo.family_id, deviceInfo.model_id, deviceInfo.version_id};

    DiscoveryReply reply(common, details, 0x7F, pimpl_->device_.getConfig().receivable_max_sysex_size, 0, false);

    send(reply);
}

void Messenger::sendEndpointInquiry(uint8_t group, uint32_t destination_muid, uint8_t status) {
    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    EndpointInquiry inquiry(common, status);
    send(inquiry);
}

void Messenger::sendInvalidateMuid(uint8_t group, uint32_t destination_muid, uint32_t target_muid) {
    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    InvalidateMUID invalidate(common, target_muid);
    send(invalidate);
}

void Messenger::sendProfileInquiry(uint8_t group, uint32_t destination_muid) {
    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    ProfileInquiry inquiry(common);
    send(inquiry);
}

void Messenger::sendSetProfileOn(uint8_t group, uint8_t address, uint32_t destination_muid,
                                    const MidiCIProfileId& profile_id, uint16_t num_channels) {
    Common common(pimpl_->device_.getMuid(), destination_muid, address, group);

    SetProfileOn set_on(common, profile_id, num_channels);
    send(set_on);
}

void Messenger::sendSetProfileOff(uint8_t group, uint8_t address, uint32_t destination_muid,
                                    const MidiCIProfileId& profile_id) {
    Common common(pimpl_->device_.getMuid(), destination_muid, address, group);

    SetProfileOff set_off(common, profile_id);
    send(set_off);
}

void Messenger::sendProfileEnabledReport(uint8_t group, uint8_t address,
                                            const MidiCIProfileId& profile_id, uint16_t num_channels) {

    Common common(pimpl_->device_.getMuid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileEnabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::sendProfileDisabledReport(uint8_t group, uint8_t address,
                                             const MidiCIProfileId& profile_id, uint16_t num_channels) {

    Common common(pimpl_->device_.getMuid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileDisabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::sendProfileAddedReport(uint8_t group, uint8_t address,
                                        const MidiCIProfileId& profile_id) {

    Common common(pimpl_->device_.getMuid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileAddedReport report(common, profile_id);
    send(report);
}

void Messenger::sendProfileRemovedReport(uint8_t group, uint8_t address,
                                          const MidiCIProfileId& profile_id) {

    Common common(pimpl_->device_.getMuid(), MIDI_CI_BROADCAST_MUID_32, address, group);

    ProfileRemovedReport report(common, profile_id);
    send(report);
}

void Messenger::sendPropertyGetCapabilities(uint8_t group, uint32_t destination_muid,
                                             uint8_t max_simultaneous_requests) {

    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    PropertyGetCapabilities capabilities(common, max_simultaneous_requests);
    send(capabilities);
}

void Messenger::sendPropertyGetData(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header) {

    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    GetPropertyData getData(common, request_id, header);
    send(getData);
}

void Messenger::sendPropertySetData(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {

    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    SetPropertyData set_data(common, request_id, header, body);
    send(set_data);
}

void Messenger::sendPropertySubscribe(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                      const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {

    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    SubscribeProperty subscribe(common, request_id, header, body);
    send(subscribe);
}

void Messenger::sendNakForError(const Common& common, uint8_t original_sub_id2, uint8_t status_code, 
                                  uint8_t status_data, const std::vector<uint8_t>& details, const std::string& message) {
    std::vector<uint8_t> dst(pimpl_->device_.getConfig().receivable_max_sysex_size);
    std::vector<uint8_t> message_bytes(message.begin(), message.end());
    
    auto nak_message = CIFactory::midiCIAckNak(
        dst, true, common.address, CI_VERSION_AND_FORMAT,
        pimpl_->device_.getMuid(), common.source_muid, original_sub_id2,
        status_code, status_data, details, message_bytes
    );
    
    auto ci_output_sender = pimpl_->device_.getCiOutputSender();
    if (ci_output_sender) {
        ci_output_sender(common.group, nak_message);
    }
}

void Messenger::sendProcessInquiryCapabilities(uint8_t group, uint32_t destination_muid) {

    Common common(pimpl_->device_.getMuid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);

    ProcessInquiryCapabilities inquiry(common);
    send(inquiry);
}

void Messenger::sendMidiMessageReportInquiry(uint8_t group, uint8_t address, uint32_t destination_muid,
                                                uint8_t message_data_control, uint8_t system_messages,
                                                uint8_t channel_controller_messages, uint8_t note_data_messages) {
    Common common(pimpl_->device_.getMuid(), destination_muid, address, group);

    MidiMessageReportInquiry inquiry(common, message_data_control, system_messages,
                                   channel_controller_messages, note_data_messages);
    send(inquiry);
}

void Messenger::processInput(uint8_t group, const std::vector<uint8_t>& data) {

    if (data.size() < 4 ||
        data[0] != MIDI_CI_UNIVERSAL_SYSEX_ID ||
        data[2] != MIDI_CI_SUB_ID_1) {
        return;
    }

    if (data.size() < MIDI_CI_COMMON_HEADER_SIZE) {
        return;
    }

    uint32_t source_muid = CIRetrieval::getSourceMuid(data);
    uint32_t dest_muid = CIRetrieval::getDestinationMuid(data);
    uint8_t address = CIRetrieval::getAddressing(data);

    Common common(source_muid, dest_muid, address, group);

    if (dest_muid != pimpl_->device_.getMuid() && dest_muid != MIDI_CI_BROADCAST_MUID_32) {
        return;
    }

    CISubId2 message_type = static_cast<CISubId2>(data[3]);

    switch (message_type) {
        case CISubId2::DISCOVERY_REPLY: {
            if (data.size() >= 30) {
                auto device_details = CIRetrieval::getDeviceDetails(data);

                uint8_t ci_supported = data[24];
                uint32_t max_sysex = CIRetrieval::getMaxSysexSize(data);
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
                uint32_t target_muid = CIRetrieval::getMuidToInvalidate(data);
                InvalidateMUID invalidate(common, target_muid);
                pimpl_->log_message(invalidate, false);
                processInvalidateMUID(invalidate);
            }
            break;
        }
        case CISubId2::PROFILE_INQUIRY_REPLY: {
            if (data.size() >= 15) {
                const auto [enabled_profiles, disabled_profiles] = CIRetrieval::getProfileSet(data);
                ProfileReply reply(common, enabled_profiles, disabled_profiles);
                pimpl_->log_message(reply, false);
                processProfileReply(reply);
            }
            break;
        }
        case CISubId2::PROFILE_ADDED_REPORT: {
            if (data.size() >= 18) {
                auto profile_id = CIRetrieval::getProfileId(data);
                ProfileAdded added(common, profile_id);
                pimpl_->log_message(added, false);
                processProfileAddedReport(added);
            }
            break;
        }
        case CISubId2::PROFILE_REMOVED_REPORT: {
            if (data.size() >= 18) {
                auto profile_id = CIRetrieval::getProfileId(data);
                ProfileRemoved removed(common, profile_id);
                pimpl_->log_message(removed, false);
                processProfileRemovedReport(removed);
            }
            break;
        }
        case CISubId2::PROFILE_ENABLED_REPORT: {
            if (data.size() >= 20) {
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
                uint16_t channels = CIRetrieval::getProfileEnabledChannels(data);
                ProfileEnabled enabled(common, profile_id, channels);
                pimpl_->log_message(enabled, false);
                processProfileEnabledReport(enabled);
            }
            break;
        }
        case CISubId2::PROFILE_DISABLED_REPORT: {
            if (data.size() >= 20) {
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
                uint16_t channels = CIRetrieval::getProfileEnabledChannels(data);
                ProfileDisabled disabled(common, profile_id, channels);
                pimpl_->log_message(disabled, false);
                processProfileDisabledReport(disabled);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_REPLY: {
            if (data.size() >= 14) {
                uint8_t max_requests = CIRetrieval::getMaxPropertyRequests(data);
                PropertyGetCapabilitiesReply reply(common, max_requests);
                pimpl_->log_message(reply, false);
                processPropertyCapabilitiesReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);
                uint16_t num_chunks = CIRetrieval::getPropertyTotalChunks(data);
                uint16_t chunk_index = CIRetrieval::getPropertyChunkIndex(data);
                std::string msg = std::format("GetPropertyDataReply Part: {} / {}", chunk_index, num_chunks);
                pimpl_->device_.getLogger()(LogData{msg, false});

                handleChunk(common, request_id, chunk_index, num_chunks, header, body,
                    [this, common, request_id](const std::vector<uint8_t>& complete_header, const std::vector<uint8_t>& complete_body) {
                        GetPropertyDataReply reply(common, request_id, complete_header, complete_body);
                        pimpl_->log_message(reply, false);
                        processGetDataReply(reply);
                    });
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                SetPropertyDataReply reply(common, request_id, header);
                pimpl_->log_message(reply, false);
                processSetDataReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_REPLY: {
            if (data.size() >= 21) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);
                SubscribePropertyReply reply(common, request_id, header, body);
                pimpl_->log_message(reply, false);
                processSubscribePropertyReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_NOTIFY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);
                uint16_t num_chunks = CIRetrieval::getPropertyTotalChunks(data);
                uint16_t chunk_index = CIRetrieval::getPropertyChunkIndex(data);
                
                handleChunk(common, request_id, chunk_index, num_chunks, header, body,
                    [this, common, request_id](const std::vector<uint8_t>& complete_header, const std::vector<uint8_t>& complete_body) {
                        SubscribeProperty notify(common, request_id, complete_header, complete_body);
                        pimpl_->log_message(notify, false);
                        processPropertyNotify(notify);
                    });
            }
            break;
        }
        case CISubId2::DISCOVERY_INQUIRY: {
            if (data.size() >= 30) {
                auto device_details = CIRetrieval::getDeviceDetails(data);

                uint8_t ci_supported = data[24];
                uint32_t max_sysex = CIRetrieval::getMaxSysexSize(data);
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
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
                uint16_t channels = CIRetrieval::getProfileEnabledChannels(data);
                SetProfileOn set_on(common, profile_id, channels);
                pimpl_->log_message(set_on, false);
                processSetProfileOn(set_on);
            }
            break;
        }
        case CISubId2::PROFILE_SET_OFF: {
            if (data.size() >= 18) {
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
                SetProfileOff set_off(common, profile_id);
                pimpl_->log_message(set_off, false);
                processSetProfileOff(set_off);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY: {
            if (data.size() >= 14) {
                uint8_t max_requests = CIRetrieval::getMaxPropertyRequests(data);
                PropertyGetCapabilities inquiry(common, max_requests);
                pimpl_->log_message(inquiry, false);
                processPropertyCapabilitiesInquiry(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                GetPropertyData inquiry(common, request_id, header);
                pimpl_->log_message(inquiry, false);
                processGetPropertyData(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);
                uint16_t num_chunks = CIRetrieval::getPropertyTotalChunks(data);
                uint16_t chunk_index = CIRetrieval::getPropertyChunkIndex(data);

                handleChunk(common, request_id, chunk_index, num_chunks, header, body,
                    [this, common, request_id](const std::vector<uint8_t>& complete_header, const std::vector<uint8_t>& complete_body) {
                        SetPropertyData inquiry(common, request_id, complete_header, complete_body);
                        pimpl_->log_message(inquiry, false);
                        processSetPropertyData(inquiry);
                    });
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                std::vector<uint8_t> header = CIRetrieval::getPropertyHeader(data);
                std::vector<uint8_t> body = CIRetrieval::getPropertyBodyInThisChunk(data);
                uint16_t num_chunks = CIRetrieval::getPropertyTotalChunks(data);
                uint16_t chunk_index = CIRetrieval::getPropertyChunkIndex(data);
                
                handleChunk(common, request_id, chunk_index, num_chunks, header, body,
                    [this, common, request_id](const std::vector<uint8_t>& complete_header, const std::vector<uint8_t>& complete_body) {
                        SubscribeProperty inquiry(common, request_id, complete_header, complete_body);
                        pimpl_->log_message(inquiry, false);
                        processSubscribeProperty(inquiry);
                    });
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
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
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
                MidiCIProfileId profile_id = CIRetrieval::getProfileId(data);
                uint16_t data_length = CIRetrieval::getProfileSpecificDataSize(data);
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

void Messenger::addMessageCallback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->callbacks_.push_back(callback);
}

void Messenger::removeMessageCallback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = std::find_if(pimpl_->callbacks_.begin(), pimpl_->callbacks_.end(),
        [&callback](const MessageCallback& cb) {
            return cb.target_type() == callback.target_type();
        });
    if (it != pimpl_->callbacks_.end()) {
        pimpl_->callbacks_.erase(it);
    }
}

uint8_t Messenger::getNextRequestId() noexcept {
    return ++pimpl_->request_id_counter_;
}

void Messenger::handleNewEndpoint(const DiscoveryReply& msg) {
    auto existing = pimpl_->device_.getConnection(msg.getSourceMuid());
    if (existing) {
        pimpl_->device_.removeConnection(msg.getSourceMuid());
    }

    auto connection = std::make_shared<ClientConnection>(pimpl_->device_, msg.getSourceMuid(), msg.getDeviceDetails(), msg.getMaxSysexSize());
    pimpl_->device_.storeConnection(msg.getSourceMuid(), connection);

    sendEndpointInquiry(msg.getCommon().group, msg.getSourceMuid(), 0x01);
    sendProfileInquiry(msg.getCommon().group, msg.getSourceMuid());
    sendPropertyGetCapabilities(msg.getCommon().group, msg.getSourceMuid(), 8);
    sendProcessInquiryCapabilities(msg.getCommon().group, msg.getSourceMuid());
}

template<typename MessageType, typename Func>
void Messenger::onClient(const MessageType& msg, Func func) {
    auto connection = pimpl_->device_.getConnection(msg.getSourceMuid());
    if (connection) {
        func(connection);
    }
}

EndpointReply Messenger::getEndpointReplyForInquiry(const EndpointInquiry& msg) {
    const auto& config = pimpl_->device_.getConfig();
    std::vector<uint8_t> data;

    if (msg.getStatus() == 0 && !config.product_instance_id.empty()) {
        const auto& prod_id = config.product_instance_id;
        data.assign(prod_id.begin(), prod_id.end());
    }

    return EndpointReply(
        Common(pimpl_->device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group),
        msg.getStatus(),
        data
    );
}

std::vector<ProfileReply> Messenger::getProfileRepliesForInquiry(const ProfileInquiry& msg) {
    std::vector<ProfileReply> replies;
    
    std::vector<uint8_t> addresses;
    if (msg.getCommon().address == MIDI_CI_ADDRESS_FUNCTION_BLOCK) {
        auto& profile_host = pimpl_->device_.getProfileHostFacade();
        auto& profiles = profile_host.getProfiles();
        auto all_profiles = profiles.getProfiles();
        
        std::set<uint8_t> unique_addresses;
        for (const auto& profile : all_profiles) {
            unique_addresses.insert(profile.address);
        }
        addresses.assign(unique_addresses.begin(), unique_addresses.end());
    } else {
        addresses.push_back(msg.getCommon().address);
    }
    
    for (uint8_t address : addresses) {
        auto& profile_host = pimpl_->device_.getProfileHostFacade();
        auto& profiles = profile_host.getProfiles();
        
        auto enabled_profiles = profiles.getMatchingProfiles(address, true);
        auto disabled_profiles = profiles.getMatchingProfiles(address, false);
        
        Common common(pimpl_->device_.getMuid(), msg.getSourceMuid(), address, msg.getCommon().group);
        replies.emplace_back(common, enabled_profiles, disabled_profiles);
    }
    
    return replies;
}

ProcessInquiryCapabilitiesReply Messenger::getProcessInquiryReplyFor(const ProcessInquiryCapabilities& msg) {
    return ProcessInquiryCapabilitiesReply(
        Common(pimpl_->device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group),
        0x01
    );
}

PropertyGetCapabilitiesReply Messenger::getPropertyCapabilitiesReplyFor(const PropertyGetCapabilities& msg) {
    uint8_t max_requests = std::min(msg.getMaxSimultaneousRequests(), static_cast<uint8_t>(8));
    return PropertyGetCapabilitiesReply(
        Common(pimpl_->device_.getMuid(), msg.getSourceMuid(), msg.getCommon().address, msg.getCommon().group),
        max_requests
    );
}

void Messenger::processDiscoveryReply(const DiscoveryReply& msg) {
    pimpl_->notify_callbacks(msg);
    handleNewEndpoint(msg);
}

void Messenger::processEndpointReply(const EndpointInquiry& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processInvalidateMUID(const InvalidateMUID& msg) {
    pimpl_->notify_callbacks(msg);
    pimpl_->device_.removeConnection(msg.getSourceMuid());
}

void Messenger::processProfileReply(const ProfileReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileReply(msg);
    });
}

void Messenger::processProfileAddedReport(const ProfileAdded& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileAddedReport(msg);
    });
}

void Messenger::processProfileRemovedReport(const ProfileRemoved& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileRemovedReport(msg);
    });
}

void Messenger::processProfileEnabledReport(const ProfileEnabled& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileEnabledReport(msg);
    });
}

void Messenger::processProfileDisabledReport(const ProfileDisabled& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileDisabledReport(msg);
    });
}

void Messenger::processProfileDetailsReply(const ProfileDetailsReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getProfileClientFacade().processProfileDetailsReply(msg);
    });
}

void Messenger::processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getPropertyClientFacade().processPropertyCapabilitiesReply(msg);
    });
}

void Messenger::processGetDataReply(const GetPropertyDataReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getPropertyClientFacade().processGetDataReply(msg);
    });
}

void Messenger::processSetDataReply(const SetPropertyDataReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getPropertyClientFacade().processSetDataReply(msg);
    });
}

void Messenger::processSubscribePropertyReply(const SubscribePropertyReply& msg) {
    pimpl_->notify_callbacks(msg);
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getPropertyClientFacade().processSubscribePropertyReply(msg);
    });
}

void Messenger::processPropertyNotify(const SubscribeProperty& msg) {
    pimpl_->notify_callbacks(msg);
    
    onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
        conn->getPropertyClientFacade().processSubscribeProperty(msg);
    });
}

void Messenger::processProcessInquiryReply(const ProcessInquiryCapabilitiesReply& msg) {
    pimpl_->notify_callbacks(msg);

    auto connection = pimpl_->device_.getConnection(msg.getSourceMuid());
    if (connection) {
        connection->setProcessInquirySupportedFeatures(msg.getSupportedFeatures());
    }
}

void Messenger::processDiscovery(const DiscoveryInquiry& msg) {
    pimpl_->notify_callbacks(msg);
    sendDiscoveryReply(msg.getCommon().group, msg.getSourceMuid());
}

void Messenger::processEndpointMessage(const EndpointInquiry& msg) {
    pimpl_->notify_callbacks(msg);
    send(getEndpointReplyForInquiry(msg));
}

void Messenger::processProfileInquiry(const ProfileInquiry& msg) {
    pimpl_->notify_callbacks(msg);
    auto replies = getProfileRepliesForInquiry(msg);
    for (const auto& reply : replies) {
        send(reply);
    }
}

void Messenger::processSetProfileOn(const SetProfileOn& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processSetProfileOff(const SetProfileOff& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processProfileDetailsInquiry(const ProfileDetailsReply& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processPropertyCapabilitiesInquiry(const PropertyGetCapabilities& msg) {
    pimpl_->notify_callbacks(msg);
    send(getPropertyCapabilitiesReplyFor(msg));
}

void Messenger::processGetPropertyData(const GetPropertyData& msg) {
    pimpl_->notify_callbacks(msg);
    
    auto reply = pimpl_->device_.getPropertyHostFacade().processGetPropertyData(msg);
    send(reply);
}

void Messenger::processSetPropertyData(const SetPropertyData& msg) {
    pimpl_->notify_callbacks(msg);
    auto reply = pimpl_->device_.getPropertyHostFacade().processSetPropertyData(msg);
    send(reply);
}

void Messenger::processSubscribeProperty(const SubscribeProperty& msg) {
    pimpl_->notify_callbacks(msg);
    
    // Both initiator and recipient can receive SubscribeProperty.
    auto& property_host = pimpl_->device_.getPropertyHostFacade();
    auto* property_rules = property_host.getPropertyRules();
    if (!property_rules) {
        // No property rules available, send NAK
        sendNakForError(msg.getCommon(), static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY), 
                          CI_NAK_STATUS_NAK, 0, {}, "Property rules not available");
        return;
    }
    
    auto command = property_rules->getHeaderFieldString(msg.getHeader(), "command");
    
    if (command.empty()) {
        // Missing command field - send NAK as requested
        sendNakForError(msg.getCommon(), static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY), 
                          CI_NAK_STATUS_MALFORMED_MESSAGE, 0, {}, "Missing 'command' field in SubscribeProperty");
        return;
    }
    
    if (command == "start") {
        // New subscription request - handle on host side
        auto reply = property_host.processSubscribeProperty(msg);
        send(reply);
    } else if (command == "full" || command == "partial" || command == "notify") {
        // Property value update notification - route to client side and send reply
        onClient(msg, [&](std::shared_ptr<ClientConnection> conn) {
            auto reply = conn->getPropertyClientFacade().processSubscribeProperty(msg);
            send(reply);
        });
    } else if (command == "end") {
        // End subscription - route based on whether source MUID is a known connection
        auto conn = pimpl_->device_.getConnection(msg.getCommon().source_muid);
        if (conn) {
            // Source is a known connection (subscriber) - route to client side
            auto reply = conn->getPropertyClientFacade().processSubscribeProperty(msg);
            send(reply);
        } else {
            // Source is not a known connection (notifier) - handle on host side
            auto reply = property_host.processSubscribeProperty(msg);
            send(reply);
        }
    } else {
        // Unknown command - send NAK
        sendNakForError(msg.getCommon(), static_cast<uint8_t>(CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY), 
                          CI_NAK_STATUS_MALFORMED_MESSAGE, 0, {}, "Unknown command in SubscribeProperty: " + command);
    }
}

void Messenger::processProcessInquiry(const ProcessInquiryCapabilities& msg) {
    pimpl_->notify_callbacks(msg);
    send(getProcessInquiryReplyFor(msg));
}

void Messenger::processUnknownCIMessage(const Common& common, const std::vector<uint8_t>& data) {
}

void Messenger::processMidiMessageReport(const MidiMessageReportInquiry& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processMidiMessageReportReply(const MidiMessageReportReply& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processEndOfMidiMessageReport(const MidiMessageReportNotifyEnd& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processEndpointReply(const EndpointReply& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::processAck(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text) {
}

void Messenger::processNak(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text) {
}

void Messenger::processProfileSpecificData(const ProfileSpecificData& msg) {
    pimpl_->notify_callbacks(msg);
}

void Messenger::handleChunk(const Common& common, uint8_t request_id, uint16_t chunk_index, uint16_t num_chunks,
                           const std::vector<uint8_t>& header, const std::vector<uint8_t>& body,
                           std::function<void(const std::vector<uint8_t>&, const std::vector<uint8_t>&)> on_complete) {
    PropertyChunkManager* chunk_manager = &pimpl_->local_chunk_manager_;
    auto connection = pimpl_->device_.getConnection(common.source_muid);
    if (connection) {
        auto& property_client = connection->getPropertyClientFacade();
        chunk_manager = &property_client.getPendingChunkManager();
    }

    std::vector<uint8_t> effective_header = header;
    if (effective_header.empty() && chunk_manager->hasPendingChunk(common.source_muid, request_id)) {
        effective_header = chunk_manager->getPendingHeader(common.source_muid, request_id);
    }

    pimpl_->device_.notifyPropertyChunk(common.source_muid, request_id, effective_header);
    if (chunk_index < num_chunks) {
        chunk_manager->addPendingChunk(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count(),
            common.source_muid,
            request_id,
            header,
            body
        );
    } else {
        std::vector<uint8_t> complete_header = header;
        std::vector<uint8_t> complete_body = body;

        if (chunk_manager->hasPendingChunk(common.source_muid, request_id)) {
            auto result = chunk_manager->finishPendingChunk(common.source_muid, request_id, body);
            complete_header = result.first;
            complete_body = result.second;
        }

        on_complete(complete_header, complete_body);
    }
}

} // namespace midi_ci
