#include "midicci/details/ump/UmpFactory.hpp"
#include <algorithm>

namespace midicci {
namespace ump {

Ump UmpFactory::sysex7_direct(uint8_t group, uint8_t status, uint8_t numBytes,
                              uint8_t data1, uint8_t data2, uint8_t data3,
                              uint8_t data4, uint8_t data5, uint8_t data6) {
    uint32_t int1 = (static_cast<uint32_t>(MessageType::SYSEX7) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    (static_cast<uint32_t>(status) << 16) |
                    (static_cast<uint32_t>(numBytes & 0xF) << 12);
    
    uint32_t int2 = (static_cast<uint32_t>(data1) << 24) |
                    (static_cast<uint32_t>(data2) << 16) |
                    (static_cast<uint32_t>(data3) << 8) |
                    static_cast<uint32_t>(data4);
    
    return Ump(int1, int2);
}

int UmpFactory::sysex7_get_sysex_length(const std::vector<uint8_t>& src_data) {
    int i = 0;
    while (i < static_cast<int>(src_data.size()) && src_data[i] != 0xF7) {
        i++;
    }
    // Automatically detect if 0xF0 is prepended and reduce length if it is
    return i - (src_data.size() > 0 && src_data[0] == 0xF0 ? 1 : 0);
}

int UmpFactory::sysex7_get_packet_count(const std::vector<uint8_t>& src_data) {
    int length = sysex7_get_sysex_length(src_data);
    return (length + SYSEX7_RADIX - 1) / SYSEX7_RADIX; // Ceiling division
}

Ump UmpFactory::sysex7_get_packet_of(uint8_t group, const std::vector<uint8_t>& src_data, int packet_index) {
    return sysex_get_packet_of(MessageType::SYSEX7, group, src_data, packet_index, SYSEX7_RADIX);
}

void UmpFactory::sysex7_process(uint8_t group, const std::vector<uint8_t>& src_data, 
                                std::function<void(const Ump&)> callback) {
    int packet_count = sysex7_get_packet_count(src_data);
    for (int i = 0; i < packet_count; i++) {
        callback(sysex7_get_packet_of(group, src_data, i));
    }
}

std::vector<Ump> UmpFactory::sysex7(uint8_t group, const std::vector<uint8_t>& src_data) {
    std::vector<Ump> result;
    sysex7_process(group, src_data, [&result](const Ump& ump) {
        result.push_back(ump);
    });
    return result;
}

Ump UmpFactory::sysex_get_packet_of(MessageType message_type, uint8_t group,
                                    const std::vector<uint8_t>& src_data, int packet_index, int radix) {
    int sysex_length = sysex7_get_sysex_length(src_data);
    int packet_count = (sysex_length + radix - 1) / radix;
    
    // Determine packet status
    BinaryChunkStatus status;
    if (packet_count == 1) {
        status = BinaryChunkStatus::COMPLETE_PACKET;
    } else if (packet_index == 0) {
        status = BinaryChunkStatus::START;
    } else if (packet_index == packet_count - 1) {
        status = BinaryChunkStatus::END;
    } else {
        status = BinaryChunkStatus::CONTINUE;
    }
    
    // Calculate data position, accounting for 0xF0 prefix
    int data_start = src_data.size() > 0 && src_data[0] == 0xF0 ? 1 : 0;
    int data_pos = data_start + packet_index * radix;
    
    // Calculate how many bytes are in this packet
    int remaining_bytes = sysex_length - packet_index * radix;
    int packet_bytes = std::min(remaining_bytes, radix);
    
    // Build the UMP packet
    uint32_t int1 = (static_cast<uint32_t>(message_type) << 28) |
                    (static_cast<uint32_t>(group & 0xF) << 24) |
                    (static_cast<uint32_t>(status) << 16) |
                    (static_cast<uint32_t>(packet_bytes & 0xF) << 12);
    
    uint32_t int2 = 0;
    for (int i = 0; i < packet_bytes && i < 4; i++) {
        if (data_pos + i < static_cast<int>(src_data.size())) {
            int2 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - i * 8);
        }
    }
    
    // For SysEx7, we use 2 32-bit words (8 bytes total)
    if (message_type == MessageType::SYSEX7) {
        return Ump(int1, int2);
    }
    
    // For SysEx8 (if needed later), we would use 4 32-bit words (16 bytes total)
    uint32_t int3 = 0;
    uint32_t int4 = 0;
    for (int i = 4; i < packet_bytes && i < radix; i++) {
        if (data_pos + i < static_cast<int>(src_data.size())) {
            if (i < 8) {
                int3 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - (i - 4) * 8);
            } else {
                int4 |= static_cast<uint32_t>(src_data[data_pos + i]) << (24 - (i - 8) * 8);
            }
        }
    }
    
    return Ump(int1, int2, int3, int4);
}

} // namespace ump
} // namespace midicci