#include "PlatformBackend.hpp"

#include <iostream>
#include <stdexcept>

#ifdef USE_SDL3_BACKEND
#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <imgui_impl_sdl3.h>
#endif

#ifdef USE_SDL2_BACKEND
#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_main.h>
#include <imgui_impl_sdl2.h>
#endif

#ifdef USE_GLFW_BACKEND
#include <GLFW/glfw3.h>
#include <imgui_impl_glfw.h>
#endif

#include <imgui.h>
#include <imgui_impl_opengl3.h>

namespace midicci::app {

#ifdef USE_SDL3_BACKEND

static bool g_sdl3_quit_requested = false;

class SDL3WindowingBackend : public WindowingBackend {
public:
    bool initialize() override {
        SDL_SetMainReady();
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL3 Error: " << SDL_GetError() << std::endl;
            return false;
        }
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
        return true;
    }

    WindowHandle* createWindow(const char* title, int width, int height) override {
        SDL_Window* window = SDL_CreateWindow(title, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
        if (!window) {
            std::cerr << "SDL3 Window Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        gl_context_ = SDL_GL_CreateContext(window);
        if (!gl_context_) {
            std::cerr << "SDL3 GL Context Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            return nullptr;
        }

        SDL_GL_MakeCurrent(window, gl_context_);
        SDL_GL_SetSwapInterval(1);

        current_window_ = new WindowHandle(window, WindowHandle::SDL3);
        return current_window_;
    }

    void destroyWindow(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL3) {
            SDL_DestroyWindow(window->sdlWindow);
            delete window;
            if (window == current_window_) {
                current_window_ = nullptr;
            }
        }
    }

    bool shouldClose(WindowHandle* window) override {
        return g_sdl3_quit_requested || !window || window->sdlWindow == nullptr;
    }

    void swapBuffers(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL3) {
            SDL_GL_SwapWindow(window->sdlWindow);
        }
    }

    void getDrawableSize(WindowHandle* window, int* width, int* height) override {
        if (window && window->type == WindowHandle::SDL3) {
            SDL_GetWindowSizeInPixels(window->sdlWindow, width, height);
        }
    }

    void setWindowSize(WindowHandle* window, int width, int height) override {
        if (window && window->type == WindowHandle::SDL3) {
            SDL_SetWindowSize(window->sdlWindow, width, height);
        }
    }

    void makeContextCurrent(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL3 && gl_context_) {
            SDL_GL_MakeCurrent(window->sdlWindow, gl_context_);
        }
    }

    void shutdown() override {
        if (gl_context_) {
            SDL_GL_DestroyContext(gl_context_);
            gl_context_ = nullptr;
        }
        SDL_Quit();
    }

    const char* getName() const override {
        return "SDL3";
    }

private:
    SDL_GLContext gl_context_ = nullptr;
    WindowHandle* current_window_ = nullptr;
};

class SDL3ImGuiBackend : public ImGuiPlatformBackend {
public:
    bool initialize(WindowHandle* window) override {
        if (!window || window->type != WindowHandle::SDL3) {
            return false;
        }
        return ImGui_ImplSDL3_InitForOpenGL(window->sdlWindow, SDL_GL_GetCurrentContext());
    }

    void processEvents() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT || event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                g_sdl3_quit_requested = true;
            }
        }
    }

    void newFrame() override {
        ImGui_ImplSDL3_NewFrame();
    }

    void shutdown() override {
        ImGui_ImplSDL3_Shutdown();
    }

    const char* getName() const override {
        return "SDL3 ImGui Backend";
    }
};

#endif // USE_SDL3_BACKEND

#ifdef USE_SDL2_BACKEND

static bool g_sdl2_quit_requested = false;

class SDL2WindowingBackend : public WindowingBackend {
public:
    bool initialize() override {
        SDL_SetMainReady();
        if (SDL_Init(SDL_INIT_VIDEO) < 0) {
            std::cerr << "SDL2 Error: " << SDL_GetError() << std::endl;
            return false;
        }

        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif
        return true;
    }

    WindowHandle* createWindow(const char* title, int width, int height) override {
        SDL_Window* window = SDL_CreateWindow(
            title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            width,
            height,
            SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );
        if (!window) {
            std::cerr << "SDL2 Window Error: " << SDL_GetError() << std::endl;
            return nullptr;
        }

        gl_context_ = SDL_GL_CreateContext(window);
        if (!gl_context_) {
            std::cerr << "SDL2 GL Context Error: " << SDL_GetError() << std::endl;
            SDL_DestroyWindow(window);
            return nullptr;
        }

        SDL_GL_MakeCurrent(window, gl_context_);
        SDL_GL_SetSwapInterval(1);

        current_window_ = new WindowHandle(window, WindowHandle::SDL2);
        return current_window_;
    }

    void destroyWindow(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL2) {
            SDL_DestroyWindow(window->sdlWindow);
            delete window;
            if (window == current_window_) {
                current_window_ = nullptr;
            }
        }
    }

    bool shouldClose(WindowHandle* window) override {
        return g_sdl2_quit_requested || !window || window->sdlWindow == nullptr;
    }

    void swapBuffers(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL2) {
            SDL_GL_SwapWindow(window->sdlWindow);
        }
    }

    void getDrawableSize(WindowHandle* window, int* width, int* height) override {
        if (window && window->type == WindowHandle::SDL2) {
            SDL_GL_GetDrawableSize(window->sdlWindow, width, height);
        }
    }

    void setWindowSize(WindowHandle* window, int width, int height) override {
        if (window && window->type == WindowHandle::SDL2) {
            SDL_SetWindowSize(window->sdlWindow, width, height);
        }
    }

    void makeContextCurrent(WindowHandle* window) override {
        if (window && window->type == WindowHandle::SDL2 && gl_context_) {
            SDL_GL_MakeCurrent(window->sdlWindow, gl_context_);
        }
    }

    void shutdown() override {
        if (gl_context_) {
            SDL_GL_DeleteContext(gl_context_);
            gl_context_ = nullptr;
        }
        SDL_Quit();
    }

    const char* getName() const override {
        return "SDL2";
    }

private:
    SDL_GLContext gl_context_ = nullptr;
    WindowHandle* current_window_ = nullptr;
};

class SDL2ImGuiBackend : public ImGuiPlatformBackend {
public:
    bool initialize(WindowHandle* window) override {
        if (!window || window->type != WindowHandle::SDL2) {
            return false;
        }
        return ImGui_ImplSDL2_InitForOpenGL(window->sdlWindow, SDL_GL_GetCurrentContext());
    }

    void processEvents() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) {
                g_sdl2_quit_requested = true;
            } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE) {
                g_sdl2_quit_requested = true;
            }
        }
    }

    void newFrame() override {
        ImGui_ImplSDL2_NewFrame();
    }

    void shutdown() override {
        ImGui_ImplSDL2_Shutdown();
    }

    const char* getName() const override {
        return "SDL2 ImGui Backend";
    }
};

#endif // USE_SDL2_BACKEND

#ifdef USE_GLFW_BACKEND

static bool g_glfw_quit_requested = false;

class GLFWWindowingBackend : public WindowingBackend {
public:
    bool initialize() override {
#if defined(__linux__) && !defined(__APPLE__)
#ifdef GLFW_PLATFORM_X11
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
#endif
#endif
        if (!glfwInit()) {
            std::cerr << "GLFW Error: Failed to initialize" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        return true;
    }

    WindowHandle* createWindow(const char* title, int width, int height) override {
        GLFWwindow* window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!window) {
            std::cerr << "GLFW Window Error: Failed to create window" << std::endl;
            return nullptr;
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        current_window_ = new WindowHandle(window);
        return current_window_;
    }

    void destroyWindow(WindowHandle* window) override {
        if (window && window->type == WindowHandle::GLFW) {
            glfwDestroyWindow(window->glfwWindow);
            delete window;
            if (window == current_window_) {
                current_window_ = nullptr;
            }
        }
    }

    bool shouldClose(WindowHandle* window) override {
        if (!window || window->type != WindowHandle::GLFW || !window->glfwWindow) {
            return true;
        }
        return g_glfw_quit_requested || glfwWindowShouldClose(window->glfwWindow);
    }

    void swapBuffers(WindowHandle* window) override {
        if (window && window->type == WindowHandle::GLFW) {
            glfwSwapBuffers(window->glfwWindow);
        }
    }

    void getDrawableSize(WindowHandle* window, int* width, int* height) override {
        if (window && window->type == WindowHandle::GLFW) {
            glfwGetFramebufferSize(window->glfwWindow, width, height);
        }
    }

    void setWindowSize(WindowHandle* window, int width, int height) override {
        if (window && window->type == WindowHandle::GLFW) {
            glfwSetWindowSize(window->glfwWindow, width, height);
        }
    }

    void makeContextCurrent(WindowHandle* window) override {
        if (window && window->type == WindowHandle::GLFW) {
            glfwMakeContextCurrent(window->glfwWindow);
        }
    }

    void shutdown() override {
        glfwTerminate();
    }

    const char* getName() const override {
        return "GLFW";
    }

private:
    WindowHandle* current_window_ = nullptr;
};

class GLFWImGuiBackend : public ImGuiPlatformBackend {
public:
    bool initialize(WindowHandle* window) override {
        if (!window || window->type != WindowHandle::GLFW) {
            return false;
        }
        bool ok = ImGui_ImplGlfw_InitForOpenGL(window->glfwWindow, true);
        if (ok) {
            glfwSetWindowCloseCallback(window->glfwWindow, [](GLFWwindow* w) {
                g_glfw_quit_requested = true;
                glfwSetWindowShouldClose(w, GLFW_TRUE);
            });
        }
        return ok;
    }

    void processEvents() override {
        glfwPollEvents();
    }

    void newFrame() override {
        ImGui_ImplGlfw_NewFrame();
    }

    void shutdown() override {
        ImGui_ImplGlfw_Shutdown();
    }

    const char* getName() const override {
        return "GLFW ImGui Backend";
    }
};

#endif // USE_GLFW_BACKEND

class OpenGL3Renderer : public ImGuiRenderer {
public:
    bool initialize(WindowHandle*) override {
        return ImGui_ImplOpenGL3_Init("#version 150");
    }

    void newFrame() override {
        ImGui_ImplOpenGL3_NewFrame();
    }

    void renderDrawData() override {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void shutdown() override {
        ImGui_ImplOpenGL3_Shutdown();
    }

    const char* getName() const override {
        return "OpenGL3";
    }
};

std::unique_ptr<WindowingBackend> WindowingBackend::create() {
#ifdef USE_SDL3_BACKEND
    return std::make_unique<SDL3WindowingBackend>();
#elif defined(USE_SDL2_BACKEND)
    return std::make_unique<SDL2WindowingBackend>();
#elif defined(USE_GLFW_BACKEND)
    return std::make_unique<GLFWWindowingBackend>();
#else
    throw std::runtime_error("midicci-app: no windowing backend available");
#endif
}

std::unique_ptr<ImGuiPlatformBackend> ImGuiPlatformBackend::create(WindowHandle* window) {
    if (!window) {
        return nullptr;
    }

    switch (window->type) {
#ifdef USE_SDL3_BACKEND
        case WindowHandle::SDL3:
            return std::make_unique<SDL3ImGuiBackend>();
#endif
#ifdef USE_SDL2_BACKEND
        case WindowHandle::SDL2:
            return std::make_unique<SDL2ImGuiBackend>();
#endif
#ifdef USE_GLFW_BACKEND
        case WindowHandle::GLFW:
            return std::make_unique<GLFWImGuiBackend>();
#endif
        default:
            return nullptr;
    }
}

std::unique_ptr<ImGuiRenderer> ImGuiRenderer::create() {
    return std::make_unique<OpenGL3Renderer>();
}

} // namespace midicci::app
