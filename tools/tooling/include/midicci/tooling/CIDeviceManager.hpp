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
    
    void processMidi1Input(const std::vector<uint8_t>& data, size_t start, size_t length);
    void processUmpInput(const std::vector<uint8_t>& data, size_t start, size_t length);
    void process_single_ump_packet(const umppi::Ump& ump);
    
    void setup_input_event_listener();
    
    void logMidiMessageReportChunk(const std::vector<uint8_t>& data);
    
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
