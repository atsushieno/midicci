#include "midicci/ClientConnection.hpp"
#include "midicci/Message.hpp"
#include "midicci/MidiCIConstants.hpp"
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
    return pimpl_->device_info_.get();
}

void ClientConnection::set_channel_list(const JsonValue& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->channel_list_ = std::make_unique<JsonValue>(channel_list);
}

const JsonValue* ClientConnection::get_channel_list() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->channel_list_.get();
}

void ClientConnection::set_json_schema(const JsonValue& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->json_schema_ = std::make_unique<JsonValue>(json_schema);
}

const JsonValue* ClientConnection::get_json_schema() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->json_schema_.get();
}

} // namespace
