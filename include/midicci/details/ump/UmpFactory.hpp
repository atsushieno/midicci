#pragma once

#include "midicci/details/ump/Ump.hpp"
#include <vector>
#include <functional>

namespace midicci {
namespace ump {

class UmpFactory {
public:
    // Create a single SysEx7 UMP packet directly from bytes
    static Ump sysex7_direct(uint8_t group, uint8_t status, uint8_t numBytes,
                              uint8_t data1 = 0, uint8_t data2 = 0, uint8_t data3 = 0,
                              uint8_t data4 = 0, uint8_t data5 = 0, uint8_t data6 = 0);

    // Get the length of SysEx data, automatically handling 0xF0 prefix
    static int sysex7_get_sysex_length(const std::vector<uint8_t>& src_data);

    // Calculate how many UMP packets are needed for the SysEx data
    static int sysex7_get_packet_count(const std::vector<uint8_t>& src_data);

    // Create a specific packet from SysEx data at a given index
    static Ump sysex7_get_packet_of(uint8_t group, const std::vector<uint8_t>& src_data, int packet_index);

    // Process complete SysEx data and call callback for each UMP packet
    static void sysex7_process(uint8_t group, const std::vector<uint8_t>& src_data, 
                               std::function<void(const Ump&)> callback);

    // Convenience method that returns a list of Ump objects
    static std::vector<Ump> sysex7(uint8_t group, const std::vector<uint8_t>& src_data);

private:
    // Helper method for packet creation shared by SysEx7 and SysEx8
    static Ump sysex_get_packet_of(MessageType message_type, uint8_t group,
                                   const std::vector<uint8_t>& src_data, int packet_index, int radix,
                                   bool hasStreamId, uint8_t streamId);
    
    // Constants
    static constexpr int SYSEX7_RADIX = 6; // SysEx7 can contain up to 6 bytes per packet
};

} // namespace ump
} // namespace midicci