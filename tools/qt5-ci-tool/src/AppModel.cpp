#include "AppModel.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <stdexcept>

namespace midicci::tooling::qt5 {
    using namespace midicci::tooling;

static std::unique_ptr<tooling::CIToolRepository> g_appModel;
static bool g_appInitialized = false;

void initializeAppModel() {
    if (!g_appInitialized) {
        g_appModel = std::make_unique<tooling::CIToolRepository>();
        
        auto midi_manager = g_appModel->get_midi_device_manager();
        auto ci_manager = g_appModel->get_ci_device_manager();
        
        if (midi_manager) {
            midi_manager->initialize();
        }
        if (ci_manager) {
            ci_manager->initialize();
        }
        
        g_appModel->log("Qt5 MIDI-CI Tool initialized", tooling::MessageDirection::Out);
        g_appInitialized = true;
    }
}

void shutdownAppModel() {
    if (g_appInitialized && g_appModel) {
        g_appModel->log("Qt5 MIDI-CI Tool shutting down", tooling::MessageDirection::Out);
        
        auto ci_manager = g_appModel->get_ci_device_manager();
        auto midi_manager = g_appModel->get_midi_device_manager();
        
        if (ci_manager) {
            ci_manager->shutdown();
        }
        if (midi_manager) {
            midi_manager->shutdown();
        }
        
        g_appModel.reset();
        g_appInitialized = false;
    }
}

tooling::CIToolRepository& getAppModel() {
    if (!g_appInitialized || !g_appModel) {
        throw std::runtime_error("AppModel not initialized. Call initializeAppModel() first.");
    }
    return *g_appModel;
}

} // namespace qt5_ci_tool
