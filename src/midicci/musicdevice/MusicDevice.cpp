#include "midicci/musicdevice/MusicDevice.hpp"
#include <algorithm>
#include <thread>
#include <future>

namespace midicci::musicdevice {

// CallbackMusicDeviceInputReceiver implementation
CallbackMusicDeviceInputReceiver::CallbackMusicDeviceInputReceiver(MidiInputListenerAdder listener_adder)
    : listener_adder_(listener_adder)
{
    // Set up the main input listener that distributes to all registered receivers
    listener_adder_([this](const std::vector<uint8_t>& data, size_t start, size_t length, uint64_t timestamp) {
        for (auto& receiver : input_receivers_) {
            receiver(data, start, length, timestamp);
        }
    });
}

void CallbackMusicDeviceInputReceiver::add_input_receiver(InputCallback callback) {
    input_receivers_.push_back(callback);
}

void CallbackMusicDeviceInputReceiver::remove_input_receiver(InputCallback callback) {
    // Note: Function comparison is complex, so this is a simplified implementation
    // In practice, you might want to use a different container or identification scheme
    // For now, we'll clear all receivers when remove is called
    input_receivers_.clear();
}

// CallbackMusicDeviceOutputSender implementation
CallbackMusicDeviceOutputSender::CallbackMusicDeviceOutputSender(
    std::function<void(const std::vector<uint8_t>&, size_t, size_t, uint64_t)> output_sender)
    : output_sender_(output_sender)
{
}

void CallbackMusicDeviceOutputSender::send(const std::vector<uint8_t>& bytes, size_t offset, size_t length, uint64_t timestamp_ns) {
    output_sender_(bytes, offset, length, timestamp_ns);
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
        const auto& connections = device.get_connections();
        if (!connections.empty()) {
            return connections.begin()->first;
        }
        return 0;
    };
}

void MusicDeviceConnector::connect_async(ConnectionCallback callback) {
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
    auto& ci_device = ci_session_->get_device();
    
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

void MusicDeviceConnector::send(const std::vector<uint8_t>& data, size_t offset, size_t length, uint64_t timestamp_ns) {
    sender_->send(data, offset, length, timestamp_ns);
}

void MusicDeviceConnector::set_endpoint_selector(EndpointSelector selector) {
    select_target_endpoint_ = selector;
}

void MusicDeviceConnector::set_discovery_wait(std::chrono::milliseconds wait) {
    discovery_wait_ = wait;
}

void MusicDeviceConnector::set_discovery_timeout(std::chrono::milliseconds timeout) {
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

std::shared_ptr<ClientConnection> MusicDevice::get_connection() const {
    return ci_session_->get_device().get_connection(target_muid_);
}

std::optional<DeviceInfo> MusicDevice::get_device_info() const {
    auto connection = get_connection();
    if (connection) {
        // TODO: Return device info when available from connection
        // For now, return empty optional
        return std::nullopt;
    }
    return std::nullopt;
}

void MusicDevice::send(const std::vector<uint8_t>& data, size_t offset, size_t length, uint64_t timestamp_ns) {
    sender_->send(data, offset, length, timestamp_ns);
}

} // namespace midicci::musicdevice