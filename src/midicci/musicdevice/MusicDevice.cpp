#include "midicci/midicci.hpp"
#include <algorithm>
#include <thread>
#include <future>

namespace midicci::musicdevice {

// CallbackMusicDeviceInputReceiver implementation
CallbackMusicDeviceInputReceiver::CallbackMusicDeviceInputReceiver(MidiInputListenerAdder listener_adder)
    : listener_adder_(listener_adder)
{
    // Set up the main input listener that distributes to all registered receivers
    listener_adder_([this](umppi::UmpWordSpan words, uint64_t timestamp) {
        for (auto& receiver : input_receivers_) {
            receiver(words, timestamp);
        }
    });
}

void CallbackMusicDeviceInputReceiver::addInputReceiver(InputCallback callback) {
    input_receivers_.push_back(callback);
}

void CallbackMusicDeviceInputReceiver::removeInputReceiver(InputCallback callback) {
    // Note: Function comparison is complex, so this is a simplified implementation
    // In practice, you might want to use a different container or identification scheme
    // For now, we'll clear all receivers when remove is called
    input_receivers_.clear();
}

// CallbackMusicDeviceOutputSender implementation
CallbackMusicDeviceOutputSender::CallbackMusicDeviceOutputSender(
    std::function<void(umppi::UmpWordSpan, uint64_t)> output_sender)
    : output_sender_(output_sender)
{
}

void CallbackMusicDeviceOutputSender::send(umppi::UmpWordSpan words, uint64_t timestamp_ns) {
    output_sender_(words, timestamp_ns);
}

// MusicDeviceConnector implementation
MusicDeviceConnector::MusicDeviceConnector(
    std::shared_ptr<MusicDeviceInputReceiver> receiver,
    std::shared_ptr<MusicDeviceOutputSender> sender,
    std::shared_ptr<MidiCISession> ci_session)
    : receiver_(receiver),
      sender_(sender),
      ci_session_(ci_session),
      discovery_wait_(std::chrono::milliseconds(100)),
      discovery_timeout_(std::chrono::milliseconds(10000))
{
    // Default endpoint selector: choose first available connection
    select_target_endpoint_ = [](const MidiCIDevice& device) -> uint32_t {
        const auto& connections = device.getConnections();
        if (!connections.empty()) {
            return connections.begin()->first;
        }
        return 0;
    };
}

void MusicDeviceConnector::connectAsync(ConnectionCallback callback) {
    std::thread([this, callback]() {
        try {
            auto device = connect();
            callback(std::move(device), "");
        } catch (const std::exception& e) {
            callback(nullptr, e.what());
        }
    }).detach();
}

std::unique_ptr<MusicDevice> MusicDeviceConnector::connect(std::chrono::milliseconds timeout) {
    auto& ci_device = ci_session_->getDevice();
    
    // TODO: Send discovery when sendDiscovery method is available
    // ci_device.sendDiscovery();
    
    auto start_time = std::chrono::steady_clock::now();
    
    while (true) {
        std::this_thread::sleep_for(discovery_wait_);
        
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        if (elapsed >= timeout) {
            throw std::runtime_error("MIDI-CI discovery timeout");
        }
        
        uint32_t muid = select_target_endpoint_(ci_device);
        if (muid != 0) {
            return std::make_unique<MusicDevice>(sender_, muid, ci_session_);
        }
    }
}

void MusicDeviceConnector::send(umppi::UmpWordSpan data, uint64_t timestamp_ns) {
    sender_->send(data, timestamp_ns);
}

void MusicDeviceConnector::setEndpointSelector(EndpointSelector selector) {
    select_target_endpoint_ = selector;
}

void MusicDeviceConnector::setDiscoveryWait(std::chrono::milliseconds wait) {
    discovery_wait_ = wait;
}

void MusicDeviceConnector::setDiscoveryTimeout(std::chrono::milliseconds timeout) {
    discovery_timeout_ = timeout;
}

// MusicDevice implementation
MusicDevice::MusicDevice(
    std::shared_ptr<MusicDeviceOutputSender> sender,
    uint32_t target_muid,
    std::shared_ptr<MidiCISession> ci_session)
    : sender_(sender),
      target_muid_(target_muid),
      ci_session_(ci_session)
{
}

std::shared_ptr<ClientConnection> MusicDevice::getConnection() const {
    return ci_session_->getDevice().getConnection(target_muid_);
}

std::optional<DeviceInfo> MusicDevice::getDeviceInfo() const {
    auto connection = getConnection();
    if (connection) {
        // TODO: Return device info when available from connection
        // For now, return empty optional
        return std::nullopt;
    }
    return std::nullopt;
}

void MusicDevice::send(umppi::UmpWordSpan data, uint64_t timestamp_ns) {
    sender_->send(data, timestamp_ns);
}

void MusicDevice::setPropertyBinaryGetter(PropertyBinaryGetter getter) {
    ci_session_->getDevice().getPropertyHostFacade().setPropertyBinaryGetter(std::move(getter));
}

MusicDevice::PropertyBinaryGetter MusicDevice::getPropertyBinaryGetter() const {
    return ci_session_->getDevice().getPropertyHostFacade().getPropertyBinaryGetter();
}

void MusicDevice::setPropertyBinarySetter(PropertyBinarySetter setter) {
    ci_session_->getDevice().getPropertyHostFacade().setPropertyBinarySetter(std::move(setter));
}

MusicDevice::PropertyBinarySetter MusicDevice::getPropertyBinarySetter() const {
    return ci_session_->getDevice().getPropertyHostFacade().getPropertyBinarySetter();
}

} // namespace midicci::musicdevice
