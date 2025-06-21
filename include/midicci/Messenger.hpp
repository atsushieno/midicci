#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include "Message.hpp"
#include "MidiCIConstants.hpp"

namespace midicci {
class MidiCIDevice;

class Messenger {
public:
    using MessageCallback = std::function<void(const Message&)>;
    
    explicit Messenger(MidiCIDevice& device);
    ~Messenger();
    
    Messenger(const Messenger&) = delete;
    Messenger& operator=(const Messenger&) = delete;
    
    Messenger(Messenger&&) = default;
    Messenger& operator=(Messenger&&) = default;
    
    void send(const Message& message);
    void send_discovery_inquiry(uint8_t ciCategorySupported = static_cast<uint8_t>(MidiCISupportedCategories::THREE_P));
    void send_discovery_reply(uint8_t group, uint32_t destination_muid);
    void send_endpoint_inquiry(uint8_t group, uint32_t destination_muid, uint8_t status);
    void send_invalidate_muid(uint8_t group, uint32_t destination_muid, uint32_t target_muid);
    
    void send_profile_inquiry(uint8_t group, uint32_t destination_muid);
    void send_set_profile_on(uint8_t group, uint8_t address, uint32_t destination_muid,
                             const MidiCIProfileId& profile_id, uint16_t num_channels);
    void send_set_profile_off(uint8_t group, uint8_t address, uint32_t destination_muid, 
                             const MidiCIProfileId& profile_id);
    void send_profile_enabled_report(uint8_t group, uint8_t address,
                                     const MidiCIProfileId& profile_id, uint16_t num_channels);
    void send_profile_disabled_report(uint8_t group, uint8_t address,
                                      const MidiCIProfileId& profile_id, uint16_t num_channels);
    void send_profile_added_report(uint8_t group, uint8_t address, 
                                 const MidiCIProfileId& profile_id);
    void send_profile_removed_report(uint8_t group, uint8_t address, 
                                   const MidiCIProfileId& profile_id);
    
    void send_property_get_capabilities(uint8_t group, uint32_t destination_muid, 
                                      uint8_t max_simultaneous_requests);
    void send_property_get_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                              const std::vector<uint8_t>& header);
    void send_property_set_data(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                              const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    void send_property_subscribe(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                               const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    void send_process_inquiry_capabilities(uint8_t group, uint32_t destination_muid);
    void send_midi_message_report_inquiry(uint8_t group, uint8_t address, uint32_t destination_muid,
                                        uint8_t message_data_control, uint8_t system_messages,
                                        uint8_t channel_controller_messages, uint8_t note_data_messages);
    
    void process_input(uint8_t group, const std::vector<uint8_t>& data);
    
    void add_message_callback(MessageCallback callback);
    void remove_message_callback(MessageCallback callback);
    
    uint8_t get_next_request_id() noexcept;
    
private:
    void processDiscoveryReply(const DiscoveryReply& msg);
    void processEndpointReply(const EndpointInquiry& msg);
    void processInvalidateMUID(const InvalidateMUID& msg);
    void processProfileReply(const ProfileReply& msg);
    void processProfileAddedReport(const ProfileAdded& msg);
    void processProfileRemovedReport(const ProfileRemoved& msg);
    void processProfileEnabledReport(const ProfileEnabled& msg);
    void processProfileDisabledReport(const ProfileDisabled& msg);
    void processProfileDetailsReply(const ProfileDetailsReply& msg);
    void processPropertyCapabilitiesReply(const PropertyGetCapabilitiesReply& msg);
    void processGetDataReply(const GetPropertyDataReply& msg);
    void processSetDataReply(const SetPropertyDataReply& msg);
    void processSubscribePropertyReply(const SubscribePropertyReply& msg);
    void processPropertyNotify(const SubscribeProperty& msg);
    void processProcessInquiryReply(const ProcessInquiryCapabilitiesReply& msg);
    void processDiscovery(const DiscoveryInquiry& msg);
    void processEndpointMessage(const EndpointInquiry& msg);
    void processProfileInquiry(const ProfileInquiry& msg);
    void processSetProfileOn(const SetProfileOn& msg);
    void processSetProfileOff(const SetProfileOff& msg);
    void processProfileDetailsInquiry(const ProfileDetailsReply& msg);
    void processPropertyCapabilitiesInquiry(const PropertyGetCapabilities& msg);
    void processGetPropertyData(const GetPropertyData& msg);
    void processSetPropertyData(const SetPropertyData& msg);
    void processSubscribeProperty(const SubscribeProperty& msg);
    void processProcessInquiry(const ProcessInquiryCapabilities& msg);
    void processUnknownCIMessage(const Common& common, const std::vector<uint8_t>& data);
    void processMidiMessageReport(const MidiMessageReportInquiry& msg);
    void processMidiMessageReportReply(const MidiMessageReportReply& msg);
    void processEndOfMidiMessageReport(const MidiMessageReportNotifyEnd& msg);
    void processEndpointReply(const EndpointReply& msg);
    void processAck(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text);
    void processNak(uint32_t source_muid, uint32_t dest_muid, uint8_t original_sub_id, uint8_t status_code, uint8_t status_data, const std::vector<uint8_t>& details, uint16_t message_length, const std::vector<uint8_t>& message_text);
    void processProfileSpecificData(const ProfileSpecificData& msg);
    
private:
    void handleNewEndpoint(const DiscoveryReply& msg);
    
    template<typename MessageType, typename Func>
    void onClient(const MessageType& msg, Func func);
    
    EndpointReply getEndpointReplyForInquiry(const EndpointInquiry& msg);
    std::vector<ProfileReply> getProfileRepliesForInquiry(const ProfileInquiry& msg);
    ProcessInquiryCapabilitiesReply getProcessInquiryReplyFor(const ProcessInquiryCapabilities& msg);
    PropertyGetCapabilitiesReply getPropertyCapabilitiesReplyFor(const PropertyGetCapabilities& msg);
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
