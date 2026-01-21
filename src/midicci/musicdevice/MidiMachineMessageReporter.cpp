#include "midicci/midicci.hpp"

namespace midicci::musicdevice {

MidiMachineMessageReporter::MidiMachineMessageReporter()
    : configured_midi_transport_protocol_(MidiMessageReportProtocol::Midi1Stream)
{
}

MidiMessageReportProtocol MidiMachineMessageReporter::getMidiTransportProtocol() const {
    return configured_midi_transport_protocol_;
}

void MidiMachineMessageReporter::setConfiguredMidiTransportProtocol(MidiMessageReportProtocol protocol) {
    configured_midi_transport_protocol_ = protocol;
}

} // namespace midicci::musicdevice