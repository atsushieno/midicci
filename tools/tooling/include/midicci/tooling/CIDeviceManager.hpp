#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <cstdint>
#include <mutex>
#include <midicci/midicci.hpp>
#include <midicci/details/musicdevice/MidiCISession.hpp>

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
    
private:
    CIToolRepository& repository_;
    MidiCIDeviceConfiguration& config_;
    std::shared_ptr<MidiDeviceManager> midi_device_manager_;
    std::shared_ptr<CIDeviceModel> device_model_;
    std::shared_ptr<midicci::musicdevice::MidiCISession> ci_session_;
    mutable std::recursive_mutex mutex_;
    std::vector<uint8_t> buffered_sysex7_;
    std::vector<uint8_t> buffered_sysex8_;
    
    void process_single_ump_packet(const umppi::Ump& ump);
    void setup_input_event_listener();
    void logMidiMessageReportChunk(const std::vector<uint8_t>& data);
};

} // namespace ci_tool
