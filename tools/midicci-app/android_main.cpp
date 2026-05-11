#include <SDL3/SDL_main.h>
#include "App.hpp"
#include "imgui/ImGuiApp.hpp"
#include <algorithm>
#include <cmath>

using namespace midicci::app;

int main(int argc, char** argv) {
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
