#include "midicci/details/PropertyCommonConverter.hpp"
#include <zlib.h>
#include <stdexcept>
#include <cstring>

namespace midicci::commonproperties {

std::vector<uint8_t> PropertyCommonConverter::encodeToMcoded7(const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> result;

    for (size_t i = 0; i < bytes.size(); i += 7) {
        size_t chunk_size = std::min(size_t(7), bytes.size() - i);

        uint8_t msb_byte = 0;
        for (size_t j = 0; j < chunk_size; j++) {
            msb_byte |= ((bytes[i + j] >> 7) << j);
        }
        result.push_back(msb_byte);

        for (size_t j = 0; j < chunk_size; j++) {
            result.push_back(bytes[i + j] & 0x7F);
        }
    }

    return result;
}

std::vector<uint8_t> PropertyCommonConverter::decodeMcoded7(const std::vector<uint8_t>& bytes) {
    std::vector<uint8_t> result;

    for (size_t i = 0; i < bytes.size(); i += 8) {
        if (i >= bytes.size()) break;

        uint8_t msb_byte = bytes[i];
        size_t data_bytes = std::min(size_t(7), bytes.size() - i - 1);

        for (size_t j = 0; j < data_bytes; j++) {
            uint8_t data_byte = bytes[i + 1 + j];
            uint8_t msb = (msb_byte >> j) & 0x01;
            result.push_back(data_byte | (msb << 7));
        }
    }

    return result;
}

std::vector<uint8_t> PropertyCommonConverter::encodeZlib(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }

    uLongf compressed_size = compressBound(bytes.size());
    std::vector<uint8_t> compressed(compressed_size);

    int result = compress(compressed.data(), &compressed_size,
                         bytes.data(), bytes.size());

    if (result != Z_OK) {
        throw std::runtime_error("zlib compression failed");
    }

    compressed.resize(compressed_size);
    return compressed;
}

std::vector<uint8_t> PropertyCommonConverter::decodeZlib(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return {};
    }

    uLongf uncompressed_size = bytes.size() * 4;
    std::vector<uint8_t> uncompressed;

    int result = Z_BUF_ERROR;
    while (result == Z_BUF_ERROR && uncompressed_size < bytes.size() * 1024) {
        uncompressed.resize(uncompressed_size);
        uLongf actual_size = uncompressed_size;
        result = uncompress(uncompressed.data(), &actual_size,
                           bytes.data(), bytes.size());

        if (result == Z_OK) {
            uncompressed.resize(actual_size);
            return uncompressed;
        } else if (result == Z_BUF_ERROR) {
            uncompressed_size *= 2;
        }
    }

    throw std::runtime_error("zlib decompression failed");
}

std::vector<uint8_t> PropertyCommonConverter::decodeZlibMcoded7(const std::vector<uint8_t>& body) {
    auto mcoded7_decoded = decodeMcoded7(body);
    return decodeZlib(mcoded7_decoded);
}

std::vector<uint8_t> PropertyCommonConverter::encodeToZlibMcoded7(const std::vector<uint8_t>& data) {
    auto zlib_encoded = encodeZlib(data);
    return encodeToMcoded7(zlib_encoded);
}

} // namespace midicci::commonproperties
