#include "App.hpp"
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif
#include "imgui/ImGuiApp.hpp"

using namespace midicci::app;

int main() {
    MidicciApplication app;

    ImGuiAppConfig config;
    config.windowTitle = "MIDICCI App";
    config.windowWidth = 720;
    config.windowHeight = 720;
    config.clearColor = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);

    return ImGuiApp::run(
        config,
        [&](ImGuiEventLoop*) {
            return app.initialize();
        },
        [&](ImGuiEventLoop*, WindowHandle*) {
            return app.render_frame();
        },
        [&]() {
            app.shutdown();
        }
    );
}
