#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>

namespace midi_ci {
namespace core {
class MidiCIDevice;
}
}

namespace ci_tool {

class CIToolRepository;
class MidiDeviceManager;
class CIDeviceModel;

class CIDeviceManager {
public:
    explicit CIDeviceManager(CIToolRepository& repository, 
                           std::shared_ptr<MidiDeviceManager> midi_manager);
    ~CIDeviceManager();
    
    CIDeviceManager(const CIDeviceManager&) = delete;
    CIDeviceManager& operator=(const CIDeviceManager&) = delete;
    
    void initialize();
    void shutdown();
    
    std::shared_ptr<CIDeviceModel> get_device_model() const;
    
    void process_midi1_input(const std::vector<uint8_t>& data, size_t start, size_t length);
    void process_ump_input(const std::vector<uint8_t>& data, size_t start, size_t length);
    
    void log_midi_message_report_chunk(const std::vector<uint8_t>& data);
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace ci_tool
