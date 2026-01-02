#pragma once

#include "ImGuiEventLoop.hpp"
#include "PlatformBackend.hpp"
#include <imgui.h>
#include <functional>
#include <memory>

#if defined(__APPLE__)
#include <OpenGL/gl3.h>
#elif defined(_WIN32)
#include <GL/gl.h>
#else
#if defined(__has_include)
#if __has_include(<GL/gl3.h>)
#include <GL/gl3.h>
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#else
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#endif

#if defined(__linux__) || defined(_WIN32)
#define MIDICCI_SKIP_GL_FRAMEBUFFER_BIND 1
#endif

namespace midicci::app {

struct ImGuiAppConfig {
    const char* windowTitle = "MIDICCI App";
    int windowWidth = 1280;
    int windowHeight = 720;
    ImVec4 clearColor = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
    bool enableKeyboard = true;
    float dpiScale = 1.0f;
};

class ImGuiApp {
public:
    static int run(
        const ImGuiAppConfig& config,
        std::function<bool(ImGuiEventLoop*)> onInit,
        std::function<bool(ImGuiEventLoop*, WindowHandle*, WindowingBackend*)> onFrame,
        std::function<void()> onShutdown = nullptr
    ) {
        auto windowingBackend = WindowingBackend::create();
        if (!windowingBackend || !windowingBackend->initialize()) {
            return EXIT_FAILURE;
        }

        auto* window = windowingBackend->createWindow(
            config.windowTitle, config.windowWidth, config.windowHeight);
        if (!window) {
            windowingBackend->shutdown();
            return EXIT_FAILURE;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        if (config.enableKeyboard) {
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        }
        ImGui::StyleColorsDark();

        float dpiScale = config.dpiScale <= 0.0f ? 1.0f : config.dpiScale;
        if (dpiScale > 1.1f && dpiScale <= 3.0f) {
            ImGuiStyle& style = ImGui::GetStyle();
            style.ScaleAllSizes(dpiScale);
        }

        auto platformBackend = ImGuiPlatformBackend::create(window);
        auto renderer = ImGuiRenderer::create();
        if (!platformBackend || !renderer) {
            ImGui::DestroyContext();
            windowingBackend->destroyWindow(window);
            windowingBackend->shutdown();
            return EXIT_FAILURE;
        }
        if (!platformBackend->initialize(window) || !renderer->initialize(window)) {
            cleanup(renderer, platformBackend, windowingBackend, window);
            return EXIT_FAILURE;
        }

        auto eventLoop = std::make_unique<ImGuiEventLoop>();
        auto* eventLoopPtr = eventLoop.get();

        if (!onInit(eventLoopPtr)) {
            cleanup(renderer, platformBackend, windowingBackend, window);
            return EXIT_FAILURE;
        }

        bool running = true;
        while (running) {
            platformBackend->processEvents();
            if (windowingBackend->shouldClose(window)) {
                break;
            }

            eventLoopPtr->processQueuedTasks();

            bindMainFramebuffer(windowingBackend, window);

            renderer->newFrame();
            platformBackend->newFrame();
            ImGui::NewFrame();

            if (!onFrame(eventLoopPtr, window, windowingBackend.get())) {
                running = false;
            }

            ImGui::Render();
            bindMainFramebuffer(windowingBackend, window);

            int displayW = 0;
            int displayH = 0;
            windowingBackend->getDrawableSize(window, &displayW, &displayH);
            glViewport(0, 0, displayW, displayH);
            glClearColor(
                config.clearColor.x * config.clearColor.w,
                config.clearColor.y * config.clearColor.w,
                config.clearColor.z * config.clearColor.w,
                config.clearColor.w
            );
            glClear(GL_COLOR_BUFFER_BIT);
            renderer->renderDrawData();
            windowingBackend->swapBuffers(window);
        }

        if (onShutdown) {
            onShutdown();
        }
        cleanup(renderer, platformBackend, windowingBackend, window);
        return EXIT_SUCCESS;
    }

private:
    static void bindMainFramebuffer(std::unique_ptr<WindowingBackend>& backend, WindowHandle* window) {
        backend->makeContextCurrent(window);
#if !defined(MIDICCI_SKIP_GL_FRAMEBUFFER_BIND)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
#ifdef GL_DRAW_FRAMEBUFFER
#if !defined(MIDICCI_SKIP_GL_FRAMEBUFFER_BIND)
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
#endif
#endif
#ifdef GL_READ_FRAMEBUFFER
#if !defined(MIDICCI_SKIP_GL_FRAMEBUFFER_BIND)
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
#endif
#endif
#ifdef GL_BACK
        glDrawBuffer(GL_BACK);
        glReadBuffer(GL_BACK);
#endif
    }

    static void cleanup(
        std::unique_ptr<ImGuiRenderer>& renderer,
        std::unique_ptr<ImGuiPlatformBackend>& platform,
        std::unique_ptr<WindowingBackend>& windowing,
        WindowHandle* window
    ) {
        renderer->shutdown();
        platform->shutdown();
        ImGui::DestroyContext();
        windowing->destroyWindow(window);
        windowing->shutdown();
    }
};

} // namespace midicci::app
