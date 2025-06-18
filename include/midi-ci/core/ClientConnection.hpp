#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <functional>
#include <optional>
#include "../json/Json.hpp"
#include "../profiles/ProfileClientFacade.hpp"
#include "../properties/PropertyClientFacade.hpp"

namespace midi_ci {

namespace core {

class MidiCIDevice;

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
    using MessageCallback = std::function<void(const messages::Message&)>;
    using CIOutputSender = std::function<bool(uint8_t group, const std::vector<uint8_t>& data)>;
    
    explicit ClientConnection(MidiCIDevice& device, uint32_t target_muid);
    ~ClientConnection();
    
    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(ClientConnection&) = delete;
    
    ClientConnection(ClientConnection&&) = default;
    ClientConnection& operator=(ClientConnection&&) = default;
    
    uint32_t get_target_muid() const noexcept;
    
    void set_message_callback(MessageCallback callback);
    void set_ci_output_sender(CIOutputSender sender);
    
    void send_message(const messages::Message& message);
    void process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data);
    
    bool is_connected() const noexcept;
    void disconnect();
    
    profiles::ProfileClientFacade& get_profile_client_facade();
    const profiles::ProfileClientFacade& get_profile_client_facade() const;
    
    properties::PropertyClientFacade& get_property_client_facade();
    const properties::PropertyClientFacade& get_property_client_facade() const;
    
    void set_device_info(const DeviceInfo& device_info);
    const DeviceInfo* get_device_info() const;
    
    void set_channel_list(const json::JsonValue& channel_list);
    const json::JsonValue* get_channel_list() const;
    
    void set_json_schema(const json::JsonValue& json_schema);
    const json::JsonValue* get_json_schema() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace core
} // namespace midi_ci
