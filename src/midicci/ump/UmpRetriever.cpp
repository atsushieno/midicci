#include "midicci/ump/UmpRetriever.hpp"
#include <cstring>

namespace midicci {
namespace ump {

std::vector<uint8_t> UmpRetriever::get_sysex7_data(const std::vector<Ump>& umps) {
    std::vector<uint8_t> result;
    get_sysex7_data([&result](const std::vector<uint8_t>& data) {
        result.insert(result.end(), data.begin(), data.end());
    }, umps);
    return result;
}

void UmpRetriever::get_sysex7_data(DataOutputter outputter, const std::vector<Ump>& umps) {
    for (const auto& ump : umps) {
        if (ump.get_message_type() == MessageType::SYSEX7) {
            take_sysex7_bytes(ump, outputter, ump.get_sysex7_size());
        }
    }
}

std::vector<uint8_t> UmpRetriever::get_sysex8_data(const std::vector<Ump>& umps) {
    std::vector<uint8_t> result;
    get_sysex8_data([&result](const std::vector<uint8_t>& data) {
        result.insert(result.end(), data.begin(), data.end());
    }, umps);
    return result;
}

void UmpRetriever::get_sysex8_data(DataOutputter outputter, const std::vector<Ump>& umps) {
    for (const auto& ump : umps) {
        if (ump.get_message_type() == MessageType::SYSEX8_MDS) {
            take_sysex8_bytes(ump, outputter, ump.get_sysex8_size());
        }
    }
}

void UmpRetriever::take_sysex7_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex7_size) {
    auto bytes = ump_to_platform_bytes(ump);
    std::vector<uint8_t> sysex_data;
    
    size_t start_offset = 2;
    size_t data_size = std::min(static_cast<size_t>(sysex7_size), bytes.size() - start_offset);
    
    if (data_size > 0) {
        sysex_data.assign(bytes.begin() + start_offset, bytes.begin() + start_offset + data_size);
        outputter(sysex_data);
    }
}

void UmpRetriever::take_sysex8_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex8_size) {
    auto bytes = ump_to_platform_bytes(ump);
    std::vector<uint8_t> sysex_data;
    
    size_t start_offset = 2;
    size_t data_size = std::min(static_cast<size_t>(sysex8_size), bytes.size() - start_offset);
    
    if (data_size > 0) {
        sysex_data.assign(bytes.begin() + start_offset, bytes.begin() + start_offset + data_size);
        outputter(sysex_data);
    }
}

std::vector<uint8_t> UmpRetriever::ump_to_platform_bytes(const Ump& ump) {
    std::vector<uint8_t> bytes;
    size_t size = ump.get_size_in_bytes();
    bytes.reserve(size);
    
    auto add_int_bytes = [&bytes](uint32_t value) {
        bytes.push_back(value & 0xFF);
        bytes.push_back((value >> 8) & 0xFF);
        bytes.push_back((value >> 16) & 0xFF);
        bytes.push_back((value >> 24) & 0xFF);
    };
    
    add_int_bytes(ump.int1);
    if (size > 4) add_int_bytes(ump.int2);
    if (size > 8) add_int_bytes(ump.int3);
    if (size > 12) add_int_bytes(ump.int4);
    
    return bytes;
}

} // namespace ump
} // namespace midi_ci
