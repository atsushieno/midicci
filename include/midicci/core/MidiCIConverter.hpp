#pragma once

#include <string>

namespace midicci {
namespace core {

class MidiCIConverter {
public:
    static std::string encodeStringToASCII(const std::string& s);
    static std::string decodeASCIIToString(const std::string& s);

private:
    MidiCIConverter() = delete;
};

} // namespace core
} // namespace midi_ci
