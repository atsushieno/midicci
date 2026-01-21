#pragma once

#include <umppi/details/Ump.hpp>
#include <vector>
#include <functional>

namespace umppi {

class UmpRetriever {
public:
    using DataOutputter = std::function<void(const std::vector<uint8_t>&)>;

    static std::vector<uint8_t> getSysex7Data(const std::vector<Ump>& umps);
    static void getSysex7Data(DataOutputter outputter, const std::vector<Ump>& umps);

    static std::vector<uint8_t> getSysex8Data(const std::vector<Ump>& umps);
    static void getSysex8Data(DataOutputter outputter, const std::vector<Ump>& umps);

private:
    static void takeSysex7Bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex7_size);
    static void takeSysex8Bytes(const Ump& ump, DataOutputter outputter, uint8_t sysex8_size);
    static std::vector<uint8_t> umpToPlatformBytes(const Ump& ump);
};

} // namespace umppi
