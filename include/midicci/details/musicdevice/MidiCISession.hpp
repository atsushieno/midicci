#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include "midicci/midicci.hpp"

namespace midicci::musicdevice {

enum class MidiTransportProtocol : uint8_t {
    Midi1 = 1,
    UMP = 2
};

// Callback for receiving MIDI input: (data, start, length, timestamp_ns)
using MidiInputCallback = std::function<void(const uint8_t*, size_t, size_t, uint64_t)>;

// Callback for adding a MIDI input listener
using MidiInputListenerAdder = std::function<void(MidiInputCallback)>;

// Data class representing a MIDI I/O pair for creating a session
struct MidiCISessionSource {
    MidiTransportProtocol transport_protocol;
    MidiInputListenerAdder input_listener_adder;
    std::function<void(const uint8_t*, size_t, size_t, uint64_t)> output_sender;
    
    MidiCISessionSource(
        MidiTransportProtocol protocol,
        MidiInputListenerAdder input_adder,
        std::function<void(const uint8_t*, size_t, size_t, uint64_t)> output
    ) : transport_protocol(protocol), input_listener_adder(input_adder), output_sender(output) {}
};

// Factory function to create a MidiCISession from a source
std::unique_ptr<class MidiCISession> create_midi_ci_session(
    const MidiCISessionSource& source,
    uint32_t muid = 0,  // Will be randomly generated if 0
    MidiCIDeviceConfiguration config = MidiCIDeviceConfiguration{},
    MidiCIDevice::LoggerFunction logger = {}
);

class MidiCISession {
public:
    MidiCISession(
        MidiTransportProtocol input_protocol,
        MidiInputListenerAdder input_listener_adder,
        std::unique_ptr<MidiCIDevice> device
    );
    ~MidiCISession() = default;
    
    MidiCISession(const MidiCISession&) = delete;
    MidiCISession& operator=(const MidiCISession&) = delete;
    
    MidiCISession(MidiCISession&&) = default;
    MidiCISession& operator=(MidiCISession&&) = default;

    MidiCIDevice& get_device() { return *device_; }
    const MidiCIDevice& get_device() const { return *device_; }

private:
    void process_ci_message(uint8_t group, const std::vector<uint8_t>& data);
    void log_midi_message_report_chunk(const std::vector<uint8_t>& data);
    void process_midi1_input(const uint8_t* data, size_t start, size_t length);
    void process_ump_input(const uint8_t* data, size_t start, size_t length);
    
    std::unique_ptr<MidiCIDevice> device_;
    bool receiving_midi_message_reports_;
    uint8_t last_chunked_message_channel_;
    std::vector<uint8_t> chunked_messages_;
    std::vector<std::function<void()>> midi_message_report_mode_changed_;
    
    // UMP message buffering
    std::vector<uint8_t> buffered_sysex7_;
    std::vector<uint8_t> buffered_sysex8_;
};

} // namespace midicci::musicdevice