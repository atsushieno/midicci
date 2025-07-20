#include "midicci/musicdevice/MidiMachineMessageReporter.hpp"

namespace midicci::musicdevice {

MidiMachineMessageReporter::MidiMachineMessageReporter()
    : configured_midi_transport_protocol_(MidiMessageReportProtocol::Midi1Stream)
{
}

MidiMessageReportProtocol MidiMachineMessageReporter::get_midi_transport_protocol() const {
    return configured_midi_transport_protocol_;
}

void MidiMachineMessageReporter::set_configured_midi_transport_protocol(MidiMessageReportProtocol protocol) {
    configured_midi_transport_protocol_ = protocol;
}

} // namespace midicci::musicdevice