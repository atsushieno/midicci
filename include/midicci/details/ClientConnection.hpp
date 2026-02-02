#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <optional>
#include "midicci/midicci.hpp"

namespace midicci {

enum class SubscriptionActionState {
    Subscribing,
    Subscribed,
    Unsubscribing,
    Unsubscribed
};

struct ClientSubscription {
    std::optional<uint8_t> pendingRequestId;
    std::optional<std::string> subscriptionId;
    std::string propertyId;
    std::string resId;
    SubscriptionActionState state;
};

class ClientConnection {
public:
    using MessageCallback = std::function<void(const Message&)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit ClientConnection(MidiCIDevice& device, uint32_t target_muid, DeviceDetails target_device_details, uint32_t remote_max_sysex);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint32_t getTargetMuid() const noexcept;

    ProfileClientFacade& getProfileClientFacade();
    const ProfileClientFacade& getProfileClientFacade() const;
    
    PropertyClientFacade& getPropertyClientFacade();
    const PropertyClientFacade& getPropertyClientFacade() const;
    
    void setDeviceInfo(const DeviceInfo& device_info);
    const DeviceInfo* getDeviceInfo() const;
    
    void setChannelList(const JsonValue& channel_list);
    const JsonValue* getChannelList() const;
    
    void setJsonSchema(const JsonValue& json_schema);
    const JsonValue* getJsonSchema() const;
    
    // Convenience methods that match Kotlin pattern - use extension functions when available
    DeviceInfo deviceInfo() const;
    JsonValue channelList() const;
    JsonValue jsonSchema() const;

    uint32_t getRemoteMaxSysexSize() const;

    void setProcessInquirySupportedFeatures(uint8_t features);
    uint8_t getProcessInquirySupportedFeatures() const;
    void setAllCtrlListReceived(bool received);
    bool hasAllCtrlListReceived() const;

private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace midi_ci
