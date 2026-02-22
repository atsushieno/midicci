#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <memory>
#include <umppi/details/Ump.hpp>
#include "midicci/midicci.hpp"

namespace midicci::musicdevice {

// Callback for receiving MIDI input: (UMP words, timestamp_ns)
using MidiInputCallback = std::function<void(umppi::UmpWordSpan, uint64_t)>;

// Callback for adding a MIDI input listener
using MidiInputListenerAdder = std::function<void(MidiInputCallback)>;

// Data class representing a MIDI I/O pair for creating a session
struct MidiCISessionSource {
    MidiInputListenerAdder input_listener_adder;
    std::function<void(umppi::UmpWordSpan, uint64_t)> output_sender;
    
    MidiCISessionSource(
        MidiInputListenerAdder input_adder,
        std::function<void(umppi::UmpWordSpan, uint64_t)> output
    ) : input_listener_adder(input_adder), output_sender(output) {}
};

// Factory function to create a MidiCISession from a source
std::unique_ptr<class MidiCISession> createMidiCiSession(
    const MidiCISessionSource& source,
    uint32_t muid,
    MidiCIDeviceConfiguration& config,
    MidiCIDevice::LoggerFunction logger = {}
);

class MidiCISession {
public:
    MidiCISession(
        MidiInputListenerAdder input_listener_adder,
        std::unique_ptr<MidiCIDevice> device
    );
    ~MidiCISession() = default;
    
    MidiCISession(const MidiCISession&) = delete;
    MidiCISession& operator=(const MidiCISession&) = delete;
    
    MidiCISession(MidiCISession&&) = default;
    MidiCISession& operator=(MidiCISession&&) = default;

    MidiCIDevice& getDevice() { return *device_; }
    const MidiCIDevice& getDevice() const { return *device_; }

private:
    void processCiMessage(uint8_t group, const std::vector<uint8_t>& data);
    void logMidiMessageReportChunk(const std::vector<uint8_t>& data);
    void processUmpInput(umppi::UmpWordSpan words);
    
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
