#include "App.hpp"
#include "imgui/ImGuiApp.hpp"

#include <imgui.h>

using namespace midicci::app;

int main() {
    MidicciApplication app;

    ImGuiAppConfig config;
    config.windowTitle = "MIDICCI App";
    config.windowWidth = 1280;
    config.windowHeight = 800;
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
