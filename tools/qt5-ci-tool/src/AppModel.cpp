#include "AppModel.hpp"
#include "CIToolRepository.hpp"
#include <stdexcept>

namespace qt5_ci_tool {

static std::unique_ptr<ci_tool::CIToolRepository> g_appModel;
static bool g_appInitialized = false;

void initializeAppModel() {
    if (!g_appInitialized) {
        g_appModel = std::make_unique<ci_tool::CIToolRepository>();
        
        auto midi_manager = g_appModel->get_midi_device_manager();
        auto ci_manager = g_appModel->get_ci_device_manager();
        
        if (midi_manager) {
            midi_manager->initialize();
        }
        if (ci_manager) {
            ci_manager->initialize();
        }
        
        g_appModel->log("Qt5 MIDI-CI Tool initialized", ci_tool::MessageDirection::Out);
        g_appInitialized = true;
    }
}

void shutdownAppModel() {
    if (g_appInitialized && g_appModel) {
        g_appModel->log("Qt5 MIDI-CI Tool shutting down", ci_tool::MessageDirection::Out);
        
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

ci_tool::CIToolRepository& getAppModel() {
    if (!g_appInitialized || !g_appModel) {
        throw std::runtime_error("AppModel not initialized. Call initializeAppModel() first.");
    }
    return *g_appModel;
}

} // namespace qt5_ci_tool
