#include "midicci/midicci.hpp"
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
    if (umps.empty()) return;
    
    auto iter = umps.begin();
    while (iter != umps.end()) {
        if (iter->get_message_type() != MessageType::SYSEX7) {
            ++iter;
            continue;
        }
        
        // Process the first packet
        const auto& start_ump = *iter;
        take_sysex7_bytes(start_ump, outputter, start_ump.get_sysex7_size());
        
        // Check binary chunk status to determine if we need to continue
        BinaryChunkStatus chunk_status = start_ump.get_binary_chunk_status();
        
        if (chunk_status == BinaryChunkStatus::COMPLETE_PACKET) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::CONTINUE || chunk_status == BinaryChunkStatus::END) {
            // This shouldn't be a starter packet, but handle gracefully
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::START) {
            ++iter;
            // Process continuation packets
            while (iter != umps.end()) {
                if (iter->get_message_type() != MessageType::SYSEX7) {
                    break; // Unexpected packet type, stop processing
                }
                
                const auto& cont_ump = *iter;
                take_sysex7_bytes(cont_ump, outputter, cont_ump.get_sysex7_size());
                
                BinaryChunkStatus cont_status = cont_ump.get_binary_chunk_status();
                ++iter;
                
                if (cont_status == BinaryChunkStatus::END) {
                    break;
                } else if (cont_status == BinaryChunkStatus::CONTINUE) {
                    continue;
                } else {
                    // Unexpected status, stop processing
                    break;
                }
            }
        } else {
            ++iter;
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
    if (umps.empty()) return;
    
    auto iter = umps.begin();
    while (iter != umps.end()) {
        if (iter->get_message_type() != MessageType::SYSEX8_MDS) {
            ++iter;
            continue;
        }
        
        // Process the first packet
        const auto& start_ump = *iter;
        take_sysex8_bytes(start_ump, outputter, start_ump.get_sysex8_size());
        
        // Check binary chunk status to determine if we need to continue  
        BinaryChunkStatus chunk_status = start_ump.get_binary_chunk_status();
        
        if (chunk_status == BinaryChunkStatus::COMPLETE_PACKET) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::CONTINUE || chunk_status == BinaryChunkStatus::END) {
            // This shouldn't be a starter packet, but handle gracefully
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::START) {
            ++iter;
            // Process continuation packets
            while (iter != umps.end()) {
                if (iter->get_message_type() != MessageType::SYSEX8_MDS) {
                    break; // Unexpected packet type, stop processing
                }
                
                const auto& cont_ump = *iter;
                take_sysex8_bytes(cont_ump, outputter, cont_ump.get_sysex8_size());
                
                BinaryChunkStatus cont_status = cont_ump.get_binary_chunk_status();
                ++iter;
                
                if (cont_status == BinaryChunkStatus::END) {
                    break;
                } else if (cont_status == BinaryChunkStatus::CONTINUE) {
                    continue;
                } else {
                    // Unexpected status, stop processing
                    break;
                }
            }
        } else {
            ++iter;
        }
    }
}

void UmpRetriever::take_sysex7_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex7_size) {
    if (sysex7_size < 1)
        return;
    
    // Port from Kotlin: proper platform-specific byte extraction
    auto src = ump_to_platform_bytes(ump);
    std::vector<uint8_t> sysex_data;
    
    // Check byte order to handle platform differences correctly
    #ifdef __BYTE_ORDER__
        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            // Big endian: drop first 2 bytes, take sysex7_size bytes
            if (src.size() >= 2 && sysex7_size > 0) {
                size_t data_size = std::min(static_cast<size_t>(sysex7_size), src.size() - 2);
                sysex_data.assign(src.begin() + 2, src.begin() + 2 + data_size);
                outputter(sysex_data);
            }
        #else
            // Little endian: handle bytes in reverse order as per Kotlin implementation
            if (src.size() >= 2) {
                sysex_data.push_back(src[1]);
                if (sysex7_size > 1 && src.size() >= 2)
                    sysex_data.push_back(src[0]);
                
                if (sysex7_size > 2) {
                    // Reverse the source and take remaining bytes
                    std::vector<uint8_t> reversed(src.rbegin(), src.rend());
                    size_t remaining = std::min(static_cast<size_t>(sysex7_size - 2), reversed.size());
                    for (size_t i = 0; i < remaining; ++i) {
                        sysex_data.push_back(reversed[i]);
                    }
                }
                outputter(sysex_data);
            }
        #endif
    #else
        // Fallback: assume little endian if byte order is not defined
        if (src.size() >= 2) {
            sysex_data.push_back(src[1]);
            if (sysex7_size > 1 && src.size() >= 2)
                sysex_data.push_back(src[0]);
            
            if (sysex7_size > 2) {
                std::vector<uint8_t> reversed(src.rbegin(), src.rend());
                size_t remaining = std::min(static_cast<size_t>(sysex7_size - 2), reversed.size());
                for (size_t i = 0; i < remaining; ++i) {
                    sysex_data.push_back(reversed[i]);
                }
            }
            outputter(sysex_data);
        }
    #endif
}

void UmpRetriever::take_sysex8_bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex8_size) {
    if (sysex8_size < 2)
        return;
    
    // Port from Kotlin: proper platform-specific byte extraction
    auto src = ump_to_platform_bytes(ump);
    std::vector<uint8_t> sysex_data;
    
    // Note that sysex8Size always contains streamID which should NOT be part of the result.
    #ifdef __BYTE_ORDER__
        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            // Big endian: drop first 3 bytes, take sysex8_size bytes
            if (src.size() >= 3) {
                size_t data_size = std::min(static_cast<size_t>(sysex8_size), src.size() - 3);
                sysex_data.assign(src.begin() + 3, src.begin() + 3 + data_size);
                outputter(sysex_data);
            }
        #else
            // Little endian: handle bytes in complex reverse order as per Kotlin implementation
            if (src.size() >= 1) {
                sysex_data.push_back(src[0]);
                
                std::vector<uint8_t> reversed(src.rbegin(), src.rend());
                
                if (sysex8_size > 2) {
                    size_t take_size = (sysex8_size > 6) ? 4 : sysex8_size - 2;
                    size_t drop_size = 8;
                    for (size_t i = drop_size; i < reversed.size() && i < drop_size + take_size; ++i) {
                        sysex_data.push_back(reversed[i]);
                    }
                }
                
                if (sysex8_size > 6) {
                    size_t take_size = (sysex8_size > 10) ? 4 : sysex8_size - 6;
                    size_t drop_size = 4;
                    for (size_t i = drop_size; i < reversed.size() && i < drop_size + take_size; ++i) {
                        sysex_data.push_back(reversed[i]);
                    }
                }
                
                if (sysex8_size > 10) {
                    size_t take_size = sysex8_size - 10;
                    for (size_t i = 0; i < reversed.size() && i < take_size; ++i) {
                        sysex_data.push_back(reversed[i]);
                    }
                }
                
                outputter(sysex_data);
            }
        #endif
    #else
        // Fallback: assume little endian if byte order is not defined
        if (src.size() >= 1) {
            sysex_data.push_back(src[0]);
            
            std::vector<uint8_t> reversed(src.rbegin(), src.rend());
            
            if (sysex8_size > 2) {
                size_t take_size = (sysex8_size > 6) ? 4 : sysex8_size - 2;
                size_t drop_size = 8;
                for (size_t i = drop_size; i < reversed.size() && i < drop_size + take_size; ++i) {
                    sysex_data.push_back(reversed[i]);
                }
            }
            
            if (sysex8_size > 6) {
                size_t take_size = (sysex8_size > 10) ? 4 : sysex8_size - 6;
                size_t drop_size = 4;
                for (size_t i = drop_size; i < reversed.size() && i < drop_size + take_size; ++i) {
                    sysex_data.push_back(reversed[i]);
                }
            }
            
            if (sysex8_size > 10) {
                size_t take_size = sysex8_size - 10;
                for (size_t i = 0; i < reversed.size() && i < take_size; ++i) {
                    sysex_data.push_back(reversed[i]);
                }
            }
            
            outputter(sysex_data);
        }
    #endif
}

std::vector<uint8_t> UmpRetriever::ump_to_platform_bytes(const Ump& ump) {
    std::vector<uint8_t> bytes;
    size_t size = ump.get_size_in_bytes();
    bytes.reserve(size);
    
    // Port from Kotlin: proper platform byte order handling
    auto add_int_bytes = [&bytes](uint32_t value) {
        #ifdef __BYTE_ORDER__
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                // Little endian
                bytes.push_back((value >> 24) & 0xFF);
                bytes.push_back((value >> 16) & 0xFF);
                bytes.push_back((value >> 8) & 0xFF);
                bytes.push_back(value & 0xFF);
            #else
                // Big endian
                bytes.push_back((value >> 24) & 0xFF);
                bytes.push_back((value >> 16) & 0xFF);
                bytes.push_back((value >> 8) & 0xFF);
                bytes.push_back(value & 0xFF);
            #endif
        #else
            // Fallback: assume little endian if byte order is not defined
            bytes.push_back((value >> 24) & 0xFF);
            bytes.push_back((value >> 16) & 0xFF);
            bytes.push_back((value >> 8) & 0xFF);
            bytes.push_back(value & 0xFF);
        #endif
    };
    
    add_int_bytes(ump.int1);
    if (static_cast<int>(ump.get_message_type()) > 2)  // Match Kotlin logic
        add_int_bytes(ump.int2);
    if (static_cast<int>(ump.get_message_type()) > 4) {
        add_int_bytes(ump.int3);
        add_int_bytes(ump.int4);
    }
    
    return bytes;
}

} // namespace ump
} // namespace midi_ci
