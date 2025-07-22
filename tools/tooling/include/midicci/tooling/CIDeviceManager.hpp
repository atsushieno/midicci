#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <mutex>
#include <midicci/midicci.hpp>

namespace midicci {
class MidiCIDevice;
}

using namespace midicci;

namespace midicci::tooling {

class CIToolRepository;
class MidiDeviceManager;
class CIDeviceModel;

class CIDeviceManager {
public:
    explicit CIDeviceManager(CIToolRepository& repository,
                             MidiCIDeviceConfiguration& config,
                             std::shared_ptr<MidiDeviceManager> midi_manager);
    ~CIDeviceManager();
    
    CIDeviceManager(const CIDeviceManager&) = delete;
    CIDeviceManager& operator=(const CIDeviceManager&) = delete;
    
    void initialize();
    void shutdown();
    
    std::shared_ptr<CIDeviceModel> get_device_model() const;
    
    void process_midi1_input(const std::vector<uint8_t>& data, size_t start, size_t length);
    void process_ump_input(const std::vector<uint8_t>& data, size_t start, size_t length);
    
    void setup_input_event_listener();
    
    void log_midi_message_report_chunk(const std::vector<uint8_t>& data);
    
private:
    CIToolRepository& repository_;
    MidiCIDeviceConfiguration& config_;
    std::shared_ptr<MidiDeviceManager> midi_device_manager_;
    std::shared_ptr<CIDeviceModel> device_model_;
    mutable std::recursive_mutex mutex_;
    
    std::vector<uint8_t> buffered_sysex7_;
    std::vector<uint8_t> buffered_sysex8_;
};

} // namespace ci_tool
