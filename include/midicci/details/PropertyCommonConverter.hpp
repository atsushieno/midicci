#pragma once

#include <vector>
#include <cstdint>

namespace midicci::commonproperties {

class PropertyCommonConverter {
public:
    static std::vector<uint8_t> encode_to_mcoded7(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> decode_mcoded7(const std::vector<uint8_t>& bytes);

    static std::vector<uint8_t> encode_zlib(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> decode_zlib(const std::vector<uint8_t>& bytes);

    static std::vector<uint8_t> decode_zlib_mcoded7(const std::vector<uint8_t>& body);
    static std::vector<uint8_t> encode_to_zlib_mcoded7(const std::vector<uint8_t>& data);
};

} // namespace midicci::commonproperties
