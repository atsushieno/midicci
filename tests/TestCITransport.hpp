#pragma once

#include <midicci/midicci.hpp>
#include <libremidi/libremidi.hpp>
#include <memory>
#include <vector>
#include <cstdint>
#include <functional>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>

namespace midicci::test {

/**
 * TestCITransport provides a test infrastructure that uses real virtual UMP ports
 * to test MIDI-CI messaging over actual transports. This is more realistic than
 * the direct connection used in TestCIMediator.
 */
class TestCITransport {
public:
    TestCITransport();
    ~TestCITransport();

    // Get the two MIDI-CI devices under test
    MidiCIDevice& getDevice1() { return *device1_; }
    MidiCIDevice& getDevice2() { return *device2_; }

    // Wait for messages to be processed through the transport layer
    void processMessages(std::chrono::milliseconds timeout = std::chrono::milliseconds(100));

    // Wait for a specific condition with timeout
    bool waitForCondition(std::function<bool()> condition,
                         std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    // Explicitly pump messages from virtual ports
    void pumpMessages();

    // Get port names for external connections
    const std::string& getDevice1InputPortName() const { return device1_input_port_name_; }
    const std::string& getDevice1OutputPortName() const { return device1_output_port_name_; }
    const std::string& getDevice2InputPortName() const { return device2_input_port_name_; }
    const std::string& getDevice2OutputPortName() const { return device2_output_port_name_; }

    bool isRunnable() { return runnable; }

private:
    void setupDevice1();
    void setupDevice2();
    void connectDevices();

    void device1_send_ump(const std::vector<uint32_t>& ump_data);
    void device2_send_ump(const std::vector<uint32_t>& ump_data);

    bool runnable{true};

    MidiCIDeviceConfiguration config1_;
    MidiCIDeviceConfiguration config2_;
    std::unique_ptr<MidiCIDevice> device1_;
    std::unique_ptr<MidiCIDevice> device2_;

    // Virtual MIDI ports
    std::unique_ptr<libremidi::midi_out> device1_output_;
    std::unique_ptr<libremidi::midi_in> device1_input_;
    std::unique_ptr<libremidi::midi_out> device2_output_;
    std::unique_ptr<libremidi::midi_in> device2_input_;

    std::string device1_input_port_name_;
    std::string device1_output_port_name_;
    std::string device2_input_port_name_;
    std::string device2_output_port_name_;

    // For synchronization
    std::mutex mutex_;
    std::condition_variable cv_;
    bool message_received_{false};

    // SysEx buffering for fragmented messages
    std::vector<uint8_t> device1_sysex_buffer_;
    std::vector<uint8_t> device2_sysex_buffer_;

    volatile bool running_{true};
};

} // namespace midicci::test
