#include "midicci/ClientConnection.hpp"
#include "midicci/Message.hpp"
#include "midicci/MidiCIConstants.hpp"
#include "midicci/commonproperties/FoundationalResources.hpp"
#include "midicci/CIFactory.hpp"
#include "midicci/MidiCIDevice.hpp"
#include "midicci/ProfileClientFacade.hpp"
#include "midicci/PropertyClientFacade.hpp"
#include "midicci/Json.hpp"
#include <mutex>

namespace midicci {

class ClientConnection::Impl {
public:
    explicit Impl(uint32_t target_muid, MidiCIDevice& device, DeviceDetails& device_details, ClientConnection& conn)
        : target_muid_(target_muid), connected_(true),
          profile_client_facade_(std::make_unique<ProfileClientFacade>(device, conn)),
          property_client_facade_(std::make_unique<PropertyClientFacade>(device, conn)),
          channel_list_(nullptr), json_schema_(nullptr) {
        // those string fields are unknown at DiscoveryReply.
        device_info_ = std::make_unique<DeviceInfo>(device_details.manufacturer,
                                                    device_details.family,
                                                    device_details.modelNumber,
                                                    device_details.softwareRevisionLevel,
                                                    "", "", "", "", "");
    }
    
    uint32_t target_muid_;
    bool connected_;
    MessageCallback message_callback_;
    CIOutputSender ci_output_sender_;
    std::unique_ptr<ProfileClientFacade> profile_client_facade_;
    std::unique_ptr<PropertyClientFacade> property_client_facade_;
    std::unique_ptr<DeviceInfo> device_info_;
    std::unique_ptr<JsonValue> channel_list_;
    std::unique_ptr<JsonValue> json_schema_;
    mutable std::recursive_mutex mutex_;
};

ClientConnection::ClientConnection(MidiCIDevice& device, uint32_t target_muid, DeviceDetails device_details)
    : pimpl_(std::make_unique<Impl>(target_muid, device, device_details, *this)) {}

ClientConnection::~ClientConnection() = default;

uint32_t ClientConnection::get_target_muid() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->target_muid_;
}

ProfileClientFacade& ClientConnection::get_profile_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

const ProfileClientFacade& ClientConnection::get_profile_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

PropertyClientFacade& ClientConnection::get_property_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

const PropertyClientFacade& ClientConnection::get_property_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

void ClientConnection::set_device_info(const DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->device_info_ = std::make_unique<DeviceInfo>(device_info);
}

const DeviceInfo* ClientConnection::get_device_info() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    // Use extension function to get deviceInfo from properties, fallback to stored device_info_
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto deviceInfo = midicci::commonproperties::FoundationalResources::getDeviceInfo(*properties);
        if (deviceInfo.has_value()) {
            // Store the parsed device info for consistent pointer return
            pimpl_->device_info_ = std::make_unique<DeviceInfo>(deviceInfo.value());
            return pimpl_->device_info_.get();
        }
    }
    return pimpl_->device_info_.get();
}

void ClientConnection::set_channel_list(const JsonValue& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->channel_list_ = std::make_unique<JsonValue>(channel_list);
}

const JsonValue* ClientConnection::get_channel_list() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    // Use extension function to get channelList from properties, fallback to stored channel_list_
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto channelList = midicci::commonproperties::FoundationalResources::getChannelList(*properties);
        if (channelList.has_value()) {
            // Convert MidiCIChannelList to JsonValue and store it
            auto jsonValue = midicci::commonproperties::FoundationalResources::toJsonValue(channelList.value());
            pimpl_->channel_list_ = std::make_unique<JsonValue>(jsonValue);
            return pimpl_->channel_list_.get();
        }
    }
    return pimpl_->channel_list_.get();
}

void ClientConnection::set_json_schema(const JsonValue& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->json_schema_ = std::make_unique<JsonValue>(json_schema);
}

const JsonValue* ClientConnection::get_json_schema() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    // Use extension function to get jsonSchema from properties, fallback to stored json_schema_
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto jsonSchema = midicci::commonproperties::FoundationalResources::getJsonSchema(*properties);
        if (jsonSchema.has_value()) {
            pimpl_->json_schema_ = std::make_unique<JsonValue>(jsonSchema.value());
            return pimpl_->json_schema_.get();
        }
    }
    return pimpl_->json_schema_.get();
}

// Convenience methods that match Kotlin pattern
DeviceInfo ClientConnection::deviceInfo() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto deviceInfo = midicci::commonproperties::FoundationalResources::getDeviceInfo(*properties);
        if (deviceInfo.has_value()) {
            return deviceInfo.value();
        }
    }
    // Fallback to stored device info or default
    if (pimpl_->device_info_) {
        return *pimpl_->device_info_;
    }
    return DeviceInfo(0, 0, 0, 0, "", "", "", "", "");
}

JsonValue ClientConnection::channelList() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto channelList = midicci::commonproperties::FoundationalResources::getChannelList(*properties);
        if (channelList.has_value()) {
            return midicci::commonproperties::FoundationalResources::toJsonValue(channelList.value());
        }
    }
    // Fallback to stored channel list or empty JSON array
    if (pimpl_->channel_list_) {
        return *pimpl_->channel_list_;
    }
    return JsonValue(JsonArray{});
}

JsonValue ClientConnection::jsonSchema() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto* properties = pimpl_->property_client_facade_->get_properties();
    if (properties) {
        auto jsonSchema = midicci::commonproperties::FoundationalResources::getJsonSchema(*properties);
        if (jsonSchema.has_value()) {
            return jsonSchema.value();
        }
    }
    // Fallback to stored JSON schema or empty JSON object
    if (pimpl_->json_schema_) {
        return *pimpl_->json_schema_;
    }
    return JsonValue(JsonObject{});
}

} // namespace
