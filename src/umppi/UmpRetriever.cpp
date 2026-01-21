#include <umppi/details/UmpRetriever.hpp>
#include <cstring>

namespace umppi {

std::vector<uint8_t> UmpRetriever::getSysex7Data(const std::vector<Ump>& umps) {
    std::vector<uint8_t> result;
    getSysex7Data([&result](const std::vector<uint8_t>& data) {
        result.insert(result.end(), data.begin(), data.end());
    }, umps);
    return result;
}

void UmpRetriever::getSysex7Data(DataOutputter outputter, const std::vector<Ump>& umps) {
    if (umps.empty()) return;

    auto iter = umps.begin();
    while (iter != umps.end()) {
        if (iter->getMessageType() != MessageType::SYSEX7) {
            ++iter;
            continue;
        }

        const auto& start_ump = *iter;
        takeSysex7Bytes(start_ump, outputter, start_ump.getSysex7Size());

        BinaryChunkStatus chunk_status = start_ump.getBinaryChunkStatus();

        if (chunk_status == BinaryChunkStatus::COMPLETE_PACKET) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::CONTINUE || chunk_status == BinaryChunkStatus::END) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::START) {
            ++iter;
            while (iter != umps.end()) {
                if (iter->getMessageType() != MessageType::SYSEX7) {
                    break;
                }

                const auto& cont_ump = *iter;
                takeSysex7Bytes(cont_ump, outputter, cont_ump.getSysex7Size());

                BinaryChunkStatus cont_status = cont_ump.getBinaryChunkStatus();
                ++iter;

                if (cont_status == BinaryChunkStatus::END) {
                    break;
                } else if (cont_status == BinaryChunkStatus::CONTINUE) {
                    continue;
                } else {
                    break;
                }
            }
        } else {
            ++iter;
        }
    }
}

std::vector<uint8_t> UmpRetriever::getSysex8Data(const std::vector<Ump>& umps) {
    std::vector<uint8_t> result;
    getSysex8Data([&result](const std::vector<uint8_t>& data) {
        result.insert(result.end(), data.begin(), data.end());
    }, umps);
    return result;
}

void UmpRetriever::getSysex8Data(DataOutputter outputter, const std::vector<Ump>& umps) {
    if (umps.empty()) return;

    auto iter = umps.begin();
    while (iter != umps.end()) {
        if (iter->getMessageType() != MessageType::SYSEX8_MDS) {
            ++iter;
            continue;
        }

        const auto& start_ump = *iter;
        takeSysex8Bytes(start_ump, outputter, start_ump.getSysex8Size());

        BinaryChunkStatus chunk_status = start_ump.getBinaryChunkStatus();

        if (chunk_status == BinaryChunkStatus::COMPLETE_PACKET) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::CONTINUE || chunk_status == BinaryChunkStatus::END) {
            ++iter;
            continue;
        } else if (chunk_status == BinaryChunkStatus::START) {
            ++iter;
            while (iter != umps.end()) {
                if (iter->getMessageType() != MessageType::SYSEX8_MDS) {
                    break;
                }

                const auto& cont_ump = *iter;
                takeSysex8Bytes(cont_ump, outputter, cont_ump.getSysex8Size());

                BinaryChunkStatus cont_status = cont_ump.getBinaryChunkStatus();
                ++iter;

                if (cont_status == BinaryChunkStatus::END) {
                    break;
                } else if (cont_status == BinaryChunkStatus::CONTINUE) {
                    continue;
                } else {
                    break;
                }
            }
        } else {
            ++iter;
        }
    }
}

void UmpRetriever::takeSysex7Bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex7_size) {
    if (sysex7_size < 1)
        return;

    auto src = umpToPlatformBytes(ump);
    std::vector<uint8_t> sysex_data;

    #ifdef __BYTE_ORDER__
        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            if (src.size() >= 2 && sysex7_size > 0) {
                size_t data_size = std::min(static_cast<size_t>(sysex7_size), src.size() - 2);
                sysex_data.assign(src.begin() + 2, src.begin() + 2 + data_size);
                outputter(sysex_data);
            }
        #else
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
    #else
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

void UmpRetriever::takeSysex8Bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex8_size) {
    if (sysex8_size < 2)
        return;

    auto src = umpToPlatformBytes(ump);
    std::vector<uint8_t> sysex_data;

    #ifdef __BYTE_ORDER__
        #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            if (src.size() >= 3) {
                size_t data_size = std::min(static_cast<size_t>(sysex8_size), src.size() - 3);
                sysex_data.assign(src.begin() + 3, src.begin() + 3 + data_size);
                outputter(sysex_data);
            }
        #else
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

std::vector<uint8_t> UmpRetriever::umpToPlatformBytes(const Ump& ump) {
    std::vector<uint8_t> bytes;
    size_t size = ump.getSizeInBytes();
    bytes.reserve(size);

    auto add_int_bytes = [&bytes](uint32_t value) {
        #ifdef __BYTE_ORDER__
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
                bytes.push_back(value & 0xFF);
                bytes.push_back((value >> 8) & 0xFF);
                bytes.push_back((value >> 16) & 0xFF);
                bytes.push_back((value >> 24) & 0xFF);
            #else
                bytes.push_back((value >> 24) & 0xFF);
                bytes.push_back((value >> 16) & 0xFF);
                bytes.push_back((value >> 8) & 0xFF);
                bytes.push_back(value & 0xFF);
            #endif
        #else
            bytes.push_back((value >> 24) & 0xFF);
            bytes.push_back((value >> 16) & 0xFF);
            bytes.push_back((value >> 8) & 0xFF);
            bytes.push_back(value & 0xFF);
        #endif
    };

    add_int_bytes(ump.int1);
    if (static_cast<int>(ump.getMessageType()) > 2)
        add_int_bytes(ump.int2);
    if (static_cast<int>(ump.getMessageType()) > 4) {
        add_int_bytes(ump.int3);
        add_int_bytes(ump.int4);
    }

    return bytes;
}

} // namespace umppi
