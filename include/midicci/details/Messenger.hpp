#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include "midicci/midicci.hpp"

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
    void sendDiscoveryInquiry(uint8_t ciCategorySupported = static_cast<uint8_t>(MidiCISupportedCategories::THREE_P));
    void sendDiscoveryReply(uint8_t group, uint32_t destination_muid);
    void sendEndpointInquiry(uint8_t group, uint32_t destination_muid, uint8_t status);
    void sendInvalidateMuid(uint8_t group, uint32_t destination_muid, uint32_t target_muid);
    
    void sendProfileInquiry(uint8_t group, uint32_t destination_muid);
    void sendSetProfileOn(uint8_t group, uint8_t address, uint32_t destination_muid,
                             const MidiCIProfileId& profile_id, uint16_t num_channels);
    void sendSetProfileOff(uint8_t group, uint8_t address, uint32_t destination_muid, 
                             const MidiCIProfileId& profile_id);
    void sendProfileEnabledReport(uint8_t group, uint8_t address,
                                     const MidiCIProfileId& profile_id, uint16_t num_channels);
    void sendProfileDisabledReport(uint8_t group, uint8_t address,
                                      const MidiCIProfileId& profile_id, uint16_t num_channels);
    void sendProfileAddedReport(uint8_t group, uint8_t address, 
                                 const MidiCIProfileId& profile_id);
    void sendProfileRemovedReport(uint8_t group, uint8_t address, 
                                   const MidiCIProfileId& profile_id);
    
    void sendPropertyGetCapabilities(uint8_t group, uint32_t destination_muid, 
                                      uint8_t max_simultaneous_requests);
    void sendPropertyGetData(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                              const std::vector<uint8_t>& header);
    void sendPropertySetData(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                              const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    void sendPropertySubscribe(uint8_t group, uint32_t destination_muid, uint8_t request_id,
                               const std::vector<uint8_t>& header, const std::vector<uint8_t>& body);
    
    void sendNakForError(const Common& common, uint8_t original_sub_id2, uint8_t status_code, 
                           uint8_t status_data, const std::vector<uint8_t>& details, const std::string& message);
    
    void sendProcessInquiryCapabilities(uint8_t group, uint32_t destination_muid);
    void sendMidiMessageReportInquiry(uint8_t group, uint8_t address, uint32_t destination_muid,
                                        uint8_t message_data_control, uint8_t system_messages,
                                        uint8_t channel_controller_messages, uint8_t note_data_messages);
    
    void processInput(uint8_t group, const std::vector<uint8_t>& data);
    
    void addMessageCallback(MessageCallback callback);
    void removeMessageCallback(MessageCallback callback);
    
    uint8_t getNextRequestId() noexcept;
    
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
    
    void handleChunk(const Common& common, uint8_t request_id, uint16_t chunk_index, uint16_t num_chunks,
                     const std::vector<uint8_t>& header, const std::vector<uint8_t>& body,
                     std::function<void(const std::vector<uint8_t>&, const std::vector<uint8_t>&)> on_complete);
    
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace
