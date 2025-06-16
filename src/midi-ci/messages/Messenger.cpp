#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include "midi-ci/core/DeviceConfig.hpp"
#include <mutex>
#include <vector>
#include <functional>
#include <atomic>
#include <algorithm>

namespace midi_ci {
namespace messages {

class Messenger::Impl {
public:
    explicit Impl(core::MidiCIDevice& device) 
        : device_(device), request_id_counter_(0) {}
    
    core::MidiCIDevice& device_;
    std::vector<MessageCallback> callbacks_;
    mutable std::recursive_mutex mutex_;
    std::atomic<uint8_t> request_id_counter_;
    
    void notify_callbacks(const Message& message) {
        std::lock_guard<std::recursive_mutex> lock(mutex_);
        for (const auto& callback : callbacks_) {
            callback(message);
        }
    }
};

Messenger::Messenger(core::MidiCIDevice& device) 
    : pimpl_(std::make_unique<Impl>(device)) {}

Messenger::~Messenger() = default;

void Messenger::send(const Message& message) {
    auto serialized = message.serialize();
    if (!serialized.empty()) {
        auto ci_output_sender = pimpl_->device_.get_ci_output_sender();
        if (ci_output_sender) {
            uint8_t group = message.get_common().group;
            ci_output_sender(group, serialized);
        }
        pimpl_->notify_callbacks(message);
    }
}

void Messenger::send_discovery_inquiry(uint8_t group, uint32_t destination_muid) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    auto device_info = pimpl_->device_.get_device_info();
    
    DiscoveryInquiry inquiry(common, 
        {device_info.manufacturer, device_info.family, device_info.model, device_info.version},
        0x7F, pimpl_->device_.get_config().max_sysex_size, 0);
    
    send(inquiry);
}

void Messenger::send_discovery_reply(uint8_t group, uint32_t destination_muid) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    auto device_info = pimpl_->device_.get_device_info();
    
    DiscoveryReply reply(common, 
        {device_info.manufacturer, device_info.family, device_info.model, device_info.version},
        0x7F, pimpl_->device_.get_config().max_sysex_size, 0, 0);
    
    send(reply);
}

void Messenger::send_endpoint_inquiry(uint8_t group, uint32_t destination_muid, uint8_t status) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    EndpointInquiry inquiry(common, status);
    send(inquiry);
}

void Messenger::send_invalidate_muid(uint8_t group, uint32_t destination_muid, uint32_t target_muid) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    InvalidateMUID invalidate(common, target_muid);
    send(invalidate);
}

void Messenger::send_profile_inquiry(uint8_t group, uint32_t destination_muid) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    ProfileInquiry inquiry(common);
    send(inquiry);
}

void Messenger::send_set_profile_on(uint8_t group, uint8_t address, uint32_t destination_muid, 
                                   const std::vector<uint8_t>& profile_id, uint16_t num_channels) {
    Common common(pimpl_->device_.get_muid(), destination_muid, address, group);
    
    SetProfileOn set_on(common, profile_id, num_channels);
    send(set_on);
}

void Messenger::send_set_profile_off(uint8_t group, uint8_t address, uint32_t destination_muid, 
                                    const std::vector<uint8_t>& profile_id) {
    Common common(pimpl_->device_.get_muid(), destination_muid, address, group);
    
    SetProfileOff set_off(common, profile_id);
    send(set_off);
}

void Messenger::send_profile_enabled_report(uint8_t group, uint8_t address, 
                                          const std::vector<uint8_t>& profile_id, uint16_t num_channels) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);
    
    ProfileEnabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::send_profile_disabled_report(uint8_t group, uint8_t address, 
                                           const std::vector<uint8_t>& profile_id, uint16_t num_channels) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);
    
    ProfileDisabledReport report(common, profile_id, num_channels);
    send(report);
}

void Messenger::send_profile_added_report(uint8_t group, uint8_t address, 
                                        const std::vector<uint8_t>& profile_id) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);
    
    ProfileAddedReport report(common, profile_id);
    send(report);
}

void Messenger::send_profile_removed_report(uint8_t group, uint8_t address, 
                                          const std::vector<uint8_t>& profile_id) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), MIDI_CI_BROADCAST_MUID_32, address, group);
    
    ProfileRemovedReport report(common, profile_id);
    send(report);
}

void Messenger::send_property_get_capabilities(uint8_t group, uint32_t destination_muid, 
                                             uint8_t max_simultaneous_requests) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    PropertyGetCapabilities capabilities(common, max_simultaneous_requests);
    send(capabilities);
}

void Messenger::send_property_get_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    GetPropertyData get_data(common, request_id, header);
    send(get_data);
}

void Messenger::send_property_set_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    SetPropertyData set_data(common, request_id, header, body);
    send(set_data);
}

void Messenger::send_property_subscribe(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                                      const std::vector<uint8_t>& header, const std::vector<uint8_t>& body) {
    using namespace midi_ci::core::constants;
    
    Common common(pimpl_->device_.get_muid(), destination_muid, MIDI_CI_ADDRESS_FUNCTION_BLOCK, group);
    
    SubscribeProperty subscribe(common, request_id, header, body);
    send(subscribe);
}

void Messenger::send_process_inquiry_capabilities(uint8_t group, uint32_t destination_muid) {
    using namespace midi_ci::core::constants;
    
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
    using namespace midi_ci::core::constants;
    
    if (data.size() < 4 || data[0] != MIDI_CI_SYSEX_START || 
        data[1] != MIDI_CI_UNIVERSAL_SYSEX_ID || data[2] != MIDI_CI_SUB_ID_1) {
        return;
    }
    
    if (data.size() < MIDI_CI_COMMON_HEADER_SIZE) {
        return;
    }
    
    uint32_t source_muid = data[5] | (data[6] << 7) | (data[7] << 14) | (data[8] << 21);
    uint32_t dest_muid = data[9] | (data[10] << 7) | (data[11] << 14) | (data[12] << 21);
    uint8_t address = data[4];
    
    Common common(source_muid, dest_muid, address, group);
    
    if (dest_muid != pimpl_->device_.get_muid() && dest_muid != MIDI_CI_BROADCAST_MUID_32) {
        return;
    }
    
    CISubId2 message_type = static_cast<CISubId2>(data[3]);
    
    switch (message_type) {
        case CISubId2::DISCOVERY_REPLY: {
            if (data.size() >= 30) {
                std::string manufacturer(reinterpret_cast<const char*>(&data[13]), 3);
                std::string family(reinterpret_cast<const char*>(&data[16]), 2);  
                std::string model(reinterpret_cast<const char*>(&data[18]), 2);
                std::string version(reinterpret_cast<const char*>(&data[20]), 4);
                DeviceInfo device_info(manufacturer, family, model, version);
                
                uint8_t ci_supported = data[24];
                uint32_t max_sysex = data[25] | (data[26] << 7) | (data[27] << 14) | (data[28] << 21);
                uint8_t output_path = data.size() > 29 ? data[29] : 0;
                uint8_t function_block = data.size() > 30 ? data[30] : 0;
                
                DiscoveryReply reply(common, device_info, ci_supported, max_sysex, output_path, function_block);
                processDiscoveryReply(reply);
            }
            break;
        }
        case CISubId2::INVALIDATE_MUID: {
            if (data.size() >= 18) {
                uint32_t target_muid = data[14] | (data[15] << 7) | (data[16] << 14) | (data[17] << 21);
                InvalidateMUID invalidate(common, target_muid);
                processInvalidateMUID(invalidate);
            }
            break;
        }
        case CISubId2::PROFILE_INQUIRY_REPLY: {
            std::vector<std::vector<uint8_t>> enabled_profiles;
            std::vector<std::vector<uint8_t>> disabled_profiles;
            ProfileReply reply(common, enabled_profiles, disabled_profiles);
            processProfileReply(reply);
            break;
        }
        case CISubId2::PROFILE_ADDED_REPORT: {
            if (data.size() >= 18) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                ProfileAdded added(common, profile_id);
                processProfileAddedReport(added);
            }
            break;
        }
        case CISubId2::PROFILE_REMOVED_REPORT: {
            if (data.size() >= 18) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                ProfileRemoved removed(common, profile_id);
                processProfileRemovedReport(removed);
            }
            break;
        }
        case CISubId2::PROFILE_ENABLED_REPORT: {
            if (data.size() >= 20) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                uint16_t channels = data[18] | (data[19] << 7);
                ProfileEnabled enabled(common, profile_id, channels);
                processProfileEnabledReport(enabled);
            }
            break;
        }
        case CISubId2::PROFILE_DISABLED_REPORT: {
            if (data.size() >= 20) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                uint16_t channels = data[18] | (data[19] << 7);
                ProfileDisabled disabled(common, profile_id, channels);
                processProfileDisabledReport(disabled);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_REPLY: {
            if (data.size() >= 14) {
                uint8_t max_requests = data[13];
                PropertyGetCapabilitiesReply reply(common, max_requests);
                processPropertyCapabilitiesReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_REPLY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                std::vector<uint8_t> body(data.begin() + 16 + header_size, data.end() - 1);
                GetPropertyDataReply reply(common, request_id, header, body);
                processGetDataReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_REPLY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                SetPropertyDataReply reply(common, request_id, header);
                processSetDataReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_REPLY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                std::vector<uint8_t> body;
                SubscribePropertyReply reply(common, request_id, header, body);
                processSubscribePropertyReply(reply);
            }
            break;
        }
        case CISubId2::PROPERTY_NOTIFY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                std::vector<uint8_t> body(data.begin() + 16 + header_size, data.end() - 1);
                SubscribeProperty notify(common, request_id, header, body);
                processPropertyNotify(notify);
            }
            break;
        }
        case CISubId2::DISCOVERY_INQUIRY: {
            if (data.size() >= 30) {
                std::string manufacturer(reinterpret_cast<const char*>(&data[13]), 3);
                std::string family(reinterpret_cast<const char*>(&data[16]), 2);  
                std::string model(reinterpret_cast<const char*>(&data[18]), 2);
                std::string version(reinterpret_cast<const char*>(&data[20]), 4);
                DeviceInfo device_info(manufacturer, family, model, version);
                
                uint8_t ci_supported = data[24];
                uint32_t max_sysex = data[25] | (data[26] << 7) | (data[27] << 14) | (data[28] << 21);
                uint8_t output_path = data.size() > 29 ? data[29] : 0;
                
                DiscoveryInquiry inquiry(common, device_info, ci_supported, max_sysex, output_path);
                processDiscovery(inquiry);
            }
            break;
        }
        case CISubId2::PROFILE_INQUIRY: {
            ProfileInquiry inquiry(common);
            processProfileInquiry(inquiry);
            break;
        }
        case CISubId2::PROFILE_SET_ON: {
            if (data.size() >= 20) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                uint16_t channels = data[18] | (data[19] << 7);
                SetProfileOn set_on(common, profile_id, channels);
                processSetProfileOn(set_on);
            }
            break;
        }
        case CISubId2::PROFILE_SET_OFF: {
            if (data.size() >= 18) {
                std::vector<uint8_t> profile_id(data.begin() + 13, data.begin() + 18);
                SetProfileOff set_off(common, profile_id);
                processSetProfileOff(set_off);
            }
            break;
        }
        case CISubId2::PROPERTY_EXCHANGE_CAPABILITIES_INQUIRY: {
            if (data.size() >= 14) {
                uint8_t max_requests = data[13];
                PropertyGetCapabilities inquiry(common, max_requests);
                processPropertyCapabilitiesInquiry(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_GET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                GetPropertyData inquiry(common, request_id, header);
                processGetPropertyData(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_SET_DATA_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                std::vector<uint8_t> body(data.begin() + 16 + header_size, data.end() - 1);
                SetPropertyData inquiry(common, request_id, header, body);
                processSetPropertyData(inquiry);
            }
            break;
        }
        case CISubId2::PROPERTY_SUBSCRIPTION_INQUIRY: {
            if (data.size() >= 16) {
                uint8_t request_id = data[13];
                uint16_t header_size = data[14] | (data[15] << 7);
                std::vector<uint8_t> header(data.begin() + 16, data.begin() + 16 + header_size);
                std::vector<uint8_t> body(data.begin() + 16 + header_size, data.end() - 1);
                SubscribeProperty inquiry(common, request_id, header, body);
                processSubscribeProperty(inquiry);
            }
            break;
        }
        default:
            processUnknownCIMessage(common, data);
            break;
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

void Messenger::processDiscoveryReply(const DiscoveryReply& msg) {
}

void Messenger::processEndpointReply(const EndpointInquiry& msg) {
}

void Messenger::processInvalidateMUID(const InvalidateMUID& msg) {
}

void Messenger::processProfileReply(const ProfileReply& msg) {
}

void Messenger::processProfileAddedReport(const ProfileAdded& msg) {
}

void Messenger::processProfileRemovedReport(const ProfileRemoved& msg) {
}

void Messenger::processProfileEnabledReport(const ProfileEnabled& msg) {
}

void Messenger::processProfileDisabledReport(const ProfileDisabled& msg) {
}

void Messenger::processProfileDetailsReply(const ProfileDetailsReply& msg) {
}

void Messenger::processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg) {
}

void Messenger::processGetDataReply(const GetPropertyDataReply& msg) {
}

void Messenger::processSetDataReply(const SetPropertyDataReply& msg) {
}

void Messenger::processSubscribePropertyReply(const SubscribePropertyReply& msg) {
}

void Messenger::processPropertyNotify(const SubscribeProperty& msg) {
}

void Messenger::processProcessInquiryReply(const ProcessInquiryCapabilitiesReply& msg) {
}

void Messenger::processDiscovery(const DiscoveryInquiry& msg) {
}

void Messenger::processEndpointMessage(const EndpointInquiry& msg) {
}

void Messenger::processProfileInquiry(const ProfileInquiry& msg) {
}

void Messenger::processSetProfileOn(const SetProfileOn& msg) {
}

void Messenger::processSetProfileOff(const SetProfileOff& msg) {
}

void Messenger::processProfileDetailsInquiry(const ProfileDetailsReply& msg) {
}

void Messenger::processPropertyCapabilitiesInquiry(const PropertyGetCapabilities& msg) {
}

void Messenger::processGetPropertyData(const GetPropertyData& msg) {
}

void Messenger::processSetPropertyData(const SetPropertyData& msg) {
}

void Messenger::processSubscribeProperty(const SubscribeProperty& msg) {
}

void Messenger::processProcessInquiry(const ProcessInquiryCapabilities& msg) {
}

void Messenger::processUnknownCIMessage(const Common& common, const std::vector<uint8_t>& data) {
}

} // namespace messages
} // namespace midi_ci
