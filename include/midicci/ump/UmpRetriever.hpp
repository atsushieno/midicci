#pragma once

#include "Ump.hpp"
#include <vector>
#include <functional>

namespace midicci {
namespace ump {

class UmpRetriever {
public:
    using DataOutputter = std::function<void(const std::vector<uint8_t>&)>;
    
    static std::vector<uint8_t> get_sysex7_data(const std::vector<Ump>& umps);
    static void get_sysex7_data(DataOutputter outputter, const std::vector<Ump>& umps);
    
    static std::vector<uint8_t> get_sysex8_data(const std::vector<Ump>& umps);
    static void get_sysex8_data(DataOutputter outputter, const std::vector<Ump>& umps);

private:
    static void take_sysex7_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex7_size);
    static void take_sysex8_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex8_size);
    static std::vector<uint8_t> ump_to_platform_bytes(const Ump& ump);
};

} // namespace ump
} // namespace midi_ci
