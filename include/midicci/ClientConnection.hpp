#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <optional>
#include "Json.hpp"
#include "ProfileClientFacade.hpp"
#include "PropertyClientFacade.hpp"

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
    SubscriptionActionState state;
};

class ClientConnection {
public:
    using MessageCallback = std::function<void(const Message&)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit ClientConnection(MidiCIDevice& device, uint32_t target_muid, DeviceDetails target_device_details);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint32_t get_target_muid() const noexcept;

    ProfileClientFacade& get_profile_client_facade();
    const ProfileClientFacade& get_profile_client_facade() const;
    
    PropertyClientFacade& get_property_client_facade();
    const PropertyClientFacade& get_property_client_facade() const;
    
    void set_device_info(const DeviceInfo& device_info);
    const DeviceInfo* get_device_info() const;
    
    void set_channel_list(const JsonValue& channel_list);
    const JsonValue* get_channel_list() const;
    
    void set_json_schema(const JsonValue& json_schema);
    const JsonValue* get_json_schema() const;
    
    // Convenience methods that match Kotlin pattern - use extension functions when available
    DeviceInfo deviceInfo() const;
    JsonValue channelList() const;
    JsonValue jsonSchema() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace midi_ci
