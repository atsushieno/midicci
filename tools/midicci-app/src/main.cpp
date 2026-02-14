#include "App.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include "imgui/ImGuiApp.hpp"
#include <algorithm>
#include <cmath>

using namespace midicci::app;

int main() {
    MidicciApplication app;

    ImGuiAppConfig config;
    config.windowTitle = "MIDICCI: Virtual MIDI 2.0 Keyboard";
    config.windowWidth = 750;
    config.windowHeight = 750;
    config.clearColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    return ImGuiApp::run(
        config,
        [&](ImGuiEventLoop*) {
            return app.initialize();
        },
        [&](ImGuiEventLoop*, WindowHandle* window, WindowingBackend* backend) {
            bool keepRunning = app.render_frame();
            if (backend && window) {
                ImVec2 pendingWindowSize;
                if (app.consume_pending_window_resize(pendingWindowSize)) {
                    int width = std::max(1, static_cast<int>(std::lround(pendingWindowSize.x)));
                    int height = std::max(1, static_cast<int>(std::lround(pendingWindowSize.y)));
                    backend->setWindowSize(window, width, height);
                }
            }
            return keepRunning;
        },
        [&]() {
            app.shutdown();
        }
    );
}
