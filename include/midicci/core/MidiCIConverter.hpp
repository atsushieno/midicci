#pragma once

#include <string>

namespace midicci {

class MidiCIConverter {
public:
    static std::string encodeStringToASCII(const std::string& s);
    static std::string decodeASCIIToString(const std::string& s);

private:
    MidiCIConverter() = delete;
};

} // namespace midi_ci
