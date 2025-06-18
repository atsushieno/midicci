#pragma once

#include <cstdint>
#include <vector>
#include <functional>

namespace midicci {
namespace transport {

using SysExCallback = std::function<void(uint8_t group, const std::vector<uint8_t>& sysex_data)>;

class SysExTransport {
public:
    virtual ~SysExTransport() = default;
    
    SysExTransport(const SysExTransport&) = delete;
    SysExTransport& operator=(const SysExTransport&) = delete;
    
    SysExTransport(SysExTransport&&) = default;
    SysExTransport& operator=(SysExTransport&&) = default;
    
    virtual bool send_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data) = 0;
    
    virtual void set_sysex_callback(SysExCallback callback) = 0;

protected:
    SysExTransport() = default;
};

} // namespace transport
} // namespace midi_ci
