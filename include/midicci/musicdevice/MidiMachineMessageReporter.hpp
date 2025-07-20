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

    MidiMessageReportProtocol get_midi_transport_protocol() const;
    void set_configured_midi_transport_protocol(MidiMessageReportProtocol protocol);

    // TODO: Implement reportMidiMessages when MIDI machine classes are available
    // For now, this is a placeholder that matches the Kotlin interface
    
private:
    MidiMessageReportProtocol configured_midi_transport_protocol_;
};

} // namespace midicci::musicdevice