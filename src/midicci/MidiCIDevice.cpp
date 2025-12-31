#include "midicci/midicci.hpp"
#include <unordered_map>
#include <mutex>

namespace midicci {

class MidiCIDevice::Impl {
public:
    Impl(MidiCIDevice& device, MidiCIDeviceConfiguration&& config, uint32_t muid) : device_id_(0x7F), config_(std::move(config)), muid_(muid),
        profile_host_facade_(std::make_unique<ProfileHostFacade>(device)),
        logger_(create_nop_logger()),
        messenger_(device) {
        property_host_facade_ = std::make_unique<PropertyHostFacade>(device, config);
    }

    static LoggerFunction create_nop_logger() {
        return [](const LogData&) { /* NOP */ };
    }
    
    uint8_t device_id_;
    MidiCIDeviceConfiguration config_;
    uint32_t muid_;
    MessageCallback message_callback_;
    MessageReceivedCallback message_received_callback_;
    ConnectionsChangedCallback connections_changed_callback_;
    MidiCIDevice::CIOutputSender ci_output_sender_;
    std::unordered_map<uint32_t, std::shared_ptr<ClientConnection>> connections_;
    std::unique_ptr<ProfileHostFacade> profile_host_facade_;
    std::unique_ptr<PropertyHostFacade> property_host_facade_;
    mutable std::recursive_mutex mutex_;
    LoggerFunction logger_;
    Messenger messenger_;
    PropertyChunkCallback property_chunk_callback_;
};

MidiCIDevice::MidiCIDevice(uint32_t muid, MidiCIDeviceConfiguration& config, LoggerFunction logger) : pimpl_(std::make_unique<Impl>(*this, std::move(config), muid)) {
    if (logger) {
        set_logger(std::move(logger));
    }
}

MidiCIDevice::~MidiCIDevice() = default;

void MidiCIDevice::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->message_callback_ = std::move(callback);
}

void MidiCIDevice::set_message_received_callback(MessageReceivedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->message_received_callback_ = std::move(callback);
    
    if (pimpl_->message_received_callback_) {
        pimpl_->messenger_.add_message_callback(pimpl_->message_received_callback_);
    }
}

void MidiCIDevice::set_connections_changed_callback(ConnectionsChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_changed_callback_ = std::move(callback);
}

void MidiCIDevice::set_property_chunk_callback(PropertyChunkCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->property_chunk_callback_ = std::move(callback);
}

void MidiCIDevice::store_connection(uint32_t destination_muid, std::shared_ptr<ClientConnection> connection) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_[destination_muid] = connection;
    
    if (pimpl_->connections_changed_callback_) {
        pimpl_->connections_changed_callback_();
    }
}

void MidiCIDevice::remove_connection(uint32_t destination_muid) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_.erase(destination_muid);
    
    if (pimpl_->connections_changed_callback_) {
        pimpl_->connections_changed_callback_();
    }
}

std::shared_ptr<ClientConnection> MidiCIDevice::get_connection(uint32_t destination_muid) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto it = pimpl_->connections_.find(destination_muid);
    return (it != pimpl_->connections_.end()) ? it->second : nullptr;
}

const std::unordered_map<uint32_t, std::shared_ptr<ClientConnection>>& MidiCIDevice::get_connections() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->connections_;
}

void MidiCIDevice::processInput(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (!sysex_data.empty()) {
        pimpl_->messenger_.process_input(group, sysex_data);
    }
}

uint32_t MidiCIDevice::get_muid() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->muid_;
}

midicci::DeviceInfo& MidiCIDevice::get_device_info() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->config_.device_info;
}

midicci::MidiCIDeviceConfiguration& MidiCIDevice::get_config() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->config_;
}

void MidiCIDevice::set_sysex_sender(MidiCIDevice::CIOutputSender sender) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->ci_output_sender_ = std::move(sender);
}

MidiCIDevice::CIOutputSender MidiCIDevice::get_ci_output_sender() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->ci_output_sender_;
}

void MidiCIDevice::sendDiscovery() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    this->pimpl_->messenger_.send_discovery_inquiry();
}

ProfileHostFacade& MidiCIDevice::get_profile_host_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_host_facade_;
}

const ProfileHostFacade& MidiCIDevice::get_profile_host_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_host_facade_;
}

PropertyHostFacade& MidiCIDevice::get_property_host_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_host_facade_;
}

const PropertyHostFacade& MidiCIDevice::get_property_host_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_host_facade_;
}

void MidiCIDevice::set_logger(LoggerFunction logger) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->logger_ = logger ? std::move(logger) : pimpl_->create_nop_logger();
}

MidiCIDevice::LoggerFunction MidiCIDevice::get_logger() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->logger_;
}

Messenger& MidiCIDevice::get_messenger() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->messenger_;
}

void MidiCIDevice::notify_property_chunk(uint32_t source_muid, uint8_t request_id, const std::vector<uint8_t>& header) {
    PropertyChunkCallback callback;
    {
        std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
        callback = pimpl_->property_chunk_callback_;
    }
    if (callback) {
        callback(source_muid, request_id, header);
    }
}

} // namespace
