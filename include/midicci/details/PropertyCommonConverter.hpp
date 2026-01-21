#pragma once

#include <vector>
#include <cstdint>

namespace midicci::commonproperties {

class PropertyCommonConverter {
public:
    static std::vector<uint8_t> encodeToMcoded7(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> decodeMcoded7(const std::vector<uint8_t>& bytes);

    static std::vector<uint8_t> encodeZlib(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> decodeZlib(const std::vector<uint8_t>& bytes);

    static std::vector<uint8_t> decodeZlibMcoded7(const std::vector<uint8_t>& body);
    static std::vector<uint8_t> encodeToZlibMcoded7(const std::vector<uint8_t>& data);
};

} // namespace midicci::commonproperties
