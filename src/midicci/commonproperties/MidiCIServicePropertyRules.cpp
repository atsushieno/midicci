#include "midicci/midicci.hpp"

namespace midicci {
namespace commonproperties {

void MidiCIServicePropertyRules::add_property_catalog_updated_callback(std::function<void()> callback) {
    property_catalog_updated_callbacks_.push_back(callback);
}

} // namespace properties
} // namespace midi_ci
