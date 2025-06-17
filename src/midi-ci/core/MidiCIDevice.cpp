#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/core/DeviceConfig.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/messages/Messenger.hpp"
#include "midi-ci/transport/SysExTransport.hpp"
#include "midi-ci/profiles/ProfileHostFacade.hpp"
#include "midi-ci/properties/PropertyHostFacade.hpp"
#include <unordered_map>
#include <mutex>

namespace midi_ci {
namespace core {

class MidiCIDevice::Impl {
public:
    Impl(MidiCIDevice& device, uint32_t muid) : device_id_(0x7F), muid_(muid), initialized_(false), 
        profile_host_facade_(std::make_unique<profiles::ProfileHostFacade>(device)),
        property_host_facade_(std::make_unique<properties::PropertyHostFacade>(device)),
        messenger_(device) {}
    
    uint8_t device_id_;
    uint32_t muid_;
    bool initialized_;
    MessageCallback message_callback_;
    MessageReceivedCallback message_received_callback_;
    ConnectionsChangedCallback connections_changed_callback_;
    MidiCIDevice::CIOutputSender ci_output_sender_;
    std::unordered_map<uint8_t, std::shared_ptr<ClientConnection>> connections_;
    std::unique_ptr<profiles::ProfileHostFacade> profile_host_facade_;
    std::unique_ptr<properties::PropertyHostFacade> property_host_facade_;
    mutable std::recursive_mutex mutex_;
    LoggerFunction logger_;
    messages::Messenger messenger_;
};

MidiCIDevice::MidiCIDevice(uint32_t muid, LoggerFunction logger) : pimpl_(std::make_unique<Impl>(*this, muid)) {
    if (logger) {
        set_logger(std::move(logger));
    }
}

MidiCIDevice::~MidiCIDevice() = default;

void MidiCIDevice::initialize() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->initialized_) {
        pimpl_->initialized_ = true;
    }
}

void MidiCIDevice::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_) {
        pimpl_->connections_.clear();
        pimpl_->initialized_ = false;
    }
}

bool MidiCIDevice::is_initialized() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->initialized_;
}

void MidiCIDevice::set_device_id(uint8_t device_id) noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->device_id_ = device_id;
}

uint8_t MidiCIDevice::get_device_id() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->device_id_;
}

void MidiCIDevice::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->message_callback_ = std::move(callback);
}

void MidiCIDevice::set_message_received_callback(MessageReceivedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->message_received_callback_ = std::move(callback);
}

void MidiCIDevice::set_connections_changed_callback(ConnectionsChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_changed_callback_ = std::move(callback);
}

void MidiCIDevice::store_connection(uint8_t destination_id, std::shared_ptr<ClientConnection> connection) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_[destination_id] = connection;
    
    if (pimpl_->connections_changed_callback_) {
        pimpl_->connections_changed_callback_();
    }
}

void MidiCIDevice::remove_connection(uint8_t destination_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_.erase(destination_id);
    
    if (pimpl_->connections_changed_callback_) {
        pimpl_->connections_changed_callback_();
    }
}

std::shared_ptr<ClientConnection> MidiCIDevice::get_connection(uint8_t destination_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = pimpl_->connections_.find(destination_id);
    return (it != pimpl_->connections_.end()) ? it->second : nullptr;
}

const std::unordered_map<uint8_t, std::shared_ptr<ClientConnection>>& MidiCIDevice::get_connections() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->connections_;
}

void MidiCIDevice::processInput(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_ && !sysex_data.empty()) {
        pimpl_->messenger_.process_input(group, sysex_data);
    }
}

uint32_t MidiCIDevice::get_muid() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->muid_;
}

midi_ci::messages::DeviceInfo MidiCIDevice::get_device_info() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return midi_ci::messages::DeviceInfo("Generic MIDI-CI Device", "Default Family", "Default Model", "1.0.0");
}

midi_ci::core::DeviceConfig MidiCIDevice::get_config() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return {};
}

void MidiCIDevice::set_sysex_sender(MidiCIDevice::CIOutputSender sender) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->ci_output_sender_ = std::move(sender);
}

MidiCIDevice::CIOutputSender MidiCIDevice::get_ci_output_sender() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->ci_output_sender_;
}

void MidiCIDevice::set_sysex_transport(std::unique_ptr<midi_ci::transport::SysExTransport> transport) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
}

void MidiCIDevice::sendDiscovery() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->initialized_) {
        messages::Messenger messenger(*this);
        messenger.send_discovery_inquiry(0, 0x7F7F7F7F);
    }
}

profiles::ProfileHostFacade& MidiCIDevice::get_profile_host_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_host_facade_;
}

const profiles::ProfileHostFacade& MidiCIDevice::get_profile_host_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_host_facade_;
}

properties::PropertyHostFacade& MidiCIDevice::get_property_host_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_host_facade_;
}

const properties::PropertyHostFacade& MidiCIDevice::get_property_host_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_host_facade_;
}

void MidiCIDevice::set_logger(LoggerFunction logger) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->logger_ = std::move(logger);
}

MidiCIDevice::LoggerFunction MidiCIDevice::get_logger() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->logger_;
}

messages::Messenger& MidiCIDevice::get_messenger() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->messenger_;
}

} // namespace core
} // namespace midi_ci
