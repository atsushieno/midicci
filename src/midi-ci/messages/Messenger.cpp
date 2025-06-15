#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
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
    mutable std::mutex mutex_;
    std::atomic<uint8_t> request_id_counter_;
    
    void notify_callbacks(const Message& message) {
        std::lock_guard<std::mutex> lock(mutex_);
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
    
    if (data.size() < MIDI_CI_COMMON_HEADER_SIZE) {
        return;
    }
    
    if (data[0] != MIDI_CI_SYSEX_START || 
        data[1] != MIDI_CI_UNIVERSAL_SYSEX_ID || 
        data[3] != MIDI_CI_SUB_ID_1) {
        return;
    }
    
    uint32_t source_muid = data[6] | (data[7] << 7) | (data[8] << 14) | (data[9] << 21);
    uint32_t dest_muid = data[10] | (data[11] << 7) | (data[12] << 14) | (data[13] << 21);
    
    if (dest_muid != pimpl_->device_.get_muid() && dest_muid != MIDI_CI_BROADCAST_MUID_32) {
        return;
    }
    
    uint8_t sub_id2 = data[4];
    
    switch (static_cast<CISubId2>(sub_id2)) {
        case CISubId2::DISCOVERY_INQUIRY:
        case CISubId2::DISCOVERY_REPLY:
        case CISubId2::PROFILE_INQUIRY:
        case CISubId2::PROFILE_INQUIRY_REPLY:
        case CISubId2::PROPERTY_GET_DATA_INQUIRY:
        case CISubId2::PROPERTY_GET_DATA_REPLY:
            break;
        default:
            break;
    }
}

void Messenger::add_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->callbacks_.push_back(callback);
}

void Messenger::remove_message_callback(MessageCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
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

} // namespace messages
} // namespace midi_ci
