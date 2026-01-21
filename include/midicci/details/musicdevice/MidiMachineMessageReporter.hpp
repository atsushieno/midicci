#pragma once

#include <cstdint>

namespace midicci::musicdevice {

enum class MidiMessageReportProtocol : uint8_t {
    Midi1Stream = 1,
    Ump = 2
};

class MidiMachineMessageReporter {
public:
    MidiMachineMessageReporter();
    ~MidiMachineMessageReporter() = default;
    
    MidiMachineMessageReporter(const MidiMachineMessageReporter&) = delete;
    MidiMachineMessageReporter& operator=(const MidiMachineMessageReporter&) = delete;
    
    MidiMachineMessageReporter(MidiMachineMessageReporter&&) = default;
    MidiMachineMessageReporter& operator=(MidiMachineMessageReporter&&) = default;

    MidiMessageReportProtocol getMidiTransportProtocol() const;
    void setConfiguredMidiTransportProtocol(MidiMessageReportProtocol protocol);

    // TODO: Implement reportMidiMessages when MIDI machine classes are available
    // For now, this is a placeholder that matches the Kotlin interface
    
private:
    MidiMessageReportProtocol configured_midi_transport_protocol_;
};

} // namespace midicci::musicdevice