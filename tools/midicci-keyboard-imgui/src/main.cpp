#ifdef IMGUI_BACKEND_SDL3
#include <SDL3/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#elif defined(IMGUI_BACKEND_SDL2)
#include <SDL2/SDL.h>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#elif defined(IMGUI_BACKEND_GLFW)
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

#include "keyboard_controller.h"
#include "midi_ci_manager.h"
#include "message_logger.h"
#include <midicci/details/commonproperties/StandardProperties.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <deque>
#include <mutex>
#include <optional>

namespace {
    // UI State
    struct UIState {
        int selectedInputDevice = 0;
        int selectedOutputDevice = 0;
        int selectedMidiCIDevice = 0;
        int currentOctave = 4;
        int velocity = 80;
        bool showDemo = false;
        bool showLogsTab = true;

        // MIDI-CI state
        bool midiCIInitialized = false;
        uint32_t midiCIMuid = 0;
        std::string midiCIDeviceName;
        std::vector<MidiCIDeviceInfo> discoveredDevices;

        // Device lists
        std::vector<std::pair<std::string, std::string>> inputDevices;
        std::vector<std::pair<std::string, std::string>> outputDevices;

        // Log entries
        std::deque<midicci::keyboard::LogEntry> logEntries;
        std::mutex logMutex;
        bool autoScrollLogs = true;

        // Standard Properties
        std::vector<midicci::commonproperties::MidiCIControl> allControls;
        std::vector<midicci::commonproperties::MidiCIProgram> allPrograms;
        bool propertiesLoaded = false;
    };

    // Piano key state
    struct PianoState {
        bool keys[12] = {false}; // One octave
        const char* noteNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    };

    UIState uiState;
    PianoState pianoState;
    std::unique_ptr<KeyboardController> controller;
    std::unique_ptr<midicci::keyboard::MessageLogger> logger;
}

void DrawStandardProperties() {
    ImGui::SeparatorText("Standard Properties");

    if (uiState.selectedMidiCIDevice < 0 ||
        uiState.selectedMidiCIDevice >= (int)uiState.discoveredDevices.size()) {
        ImGui::TextDisabled("No device selected");
        return;
    }

    uint32_t selectedMuid = uiState.discoveredDevices[uiState.selectedMidiCIDevice].muid;

    if (ImGui::Button("Refresh Properties")) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "[UI] Refreshing properties for MUID: 0x" << std::hex << selectedMuid << std::dec << std::endl;
        std::cout << "========================================\n" << std::endl;

        // Request AllCtrlList
        std::cout << "[UI] Calling controller->getAllCtrlList()..." << std::endl;
        auto controls_opt = controller->getAllCtrlList(selectedMuid);

        if (controls_opt.has_value()) {
            std::cout << "[UI] ✓ AllCtrlList received: " << controls_opt.value().size() << " controls" << std::endl;
            uiState.allControls = controls_opt.value();
            if (controls_opt.value().empty()) {
                std::cout << "[UI] ⚠ WARNING: AllCtrlList is EMPTY - may be partial/incomplete data" << std::endl;
            } else {
                for (size_t i = 0; i < std::min(controls_opt.value().size(), size_t(5)); i++) {
                    std::cout << "[UI]   - Control " << i << ": " << controls_opt.value()[i].title << std::endl;
                }
            }
        } else {
            std::cout << "[UI] ✗ AllCtrlList: No data (request sent or pending)" << std::endl;
            uiState.allControls.clear();
        }

        // Request ProgramList
        std::cout << "[UI] Calling controller->getProgramList()..." << std::endl;
        auto programs_opt = controller->getProgramList(selectedMuid);

        if (programs_opt.has_value()) {
            std::cout << "[UI] ✓ ProgramList received: " << programs_opt.value().size() << " programs" << std::endl;
            uiState.allPrograms = programs_opt.value();
            for (size_t i = 0; i < std::min(programs_opt.value().size(), size_t(5)); i++) {
                std::cout << "[UI]   - Program " << i << ": " << programs_opt.value()[i].title << std::endl;
            }
        } else {
            std::cout << "[UI] ✗ ProgramList: No data (request sent or pending)" << std::endl;
            uiState.allPrograms.clear();
        }

        uiState.propertiesLoaded = true;
        std::cout << "[UI] Properties refresh completed\n" << std::endl;
    }

    if (!uiState.propertiesLoaded) {
        ImGui::TextDisabled("Click 'Refresh Properties' to load");
        return;
    }

    ImGui::Columns(2, "PropertiesColumns", true);

    // Left column: All Controls
    ImGui::Text("All Controls (%zu)", uiState.allControls.size());
    ImGui::BeginChild("ControlsList", ImVec2(0, 200), true);

    for (size_t i = 0; i < uiState.allControls.size(); i++) {
        const auto& ctrl = uiState.allControls[i];
        ImGui::PushID((int)i);

        // Format control name
        std::string name = ctrl.title.empty() ? "Control " + std::to_string(i) : ctrl.title;
        std::string type = ctrl.ctrlType;
        std::string info = name + " [" + type;

        if (!ctrl.ctrlIndex.empty()) {
            info += " #" + std::to_string(ctrl.ctrlIndex[0]);
        }
        if (ctrl.channel.has_value()) {
            info += " Ch" + std::to_string(ctrl.channel.value());
        }
        info += "]";

        ImGui::Text("%s", info.c_str());

        // Show value slider/control if we have range info
        if (!ctrl.minMax.empty() && ctrl.minMax.size() >= 2) {
            uint32_t minVal = ctrl.minMax[0];
            uint32_t maxVal = ctrl.minMax[1];
            static uint32_t value = minVal;

            if (ImGui::SliderScalar("##value", ImGuiDataType_U32, &value, &minVal, &maxVal)) {
                // Send control change
                if (type == "cc" && !ctrl.ctrlIndex.empty()) {
                    int channel = ctrl.channel.value_or(0);
                    controller->sendControlChange(channel, ctrl.ctrlIndex[0], value);
                } else if (type == "rpn" && ctrl.ctrlIndex.size() >= 2) {
                    int channel = ctrl.channel.value_or(0);
                    controller->sendRPN(channel, ctrl.ctrlIndex[0], ctrl.ctrlIndex[1], value);
                } else if (type == "nrpn" && ctrl.ctrlIndex.size() >= 2) {
                    int channel = ctrl.channel.value_or(0);
                    controller->sendNRPN(channel, ctrl.ctrlIndex[0], ctrl.ctrlIndex[1], value);
                }
            }
        }

        ImGui::PopID();
        ImGui::Separator();
    }

    if (uiState.allControls.empty()) {
        ImGui::TextDisabled("No controls available");
    }

    ImGui::EndChild();

    ImGui::NextColumn();

    // Right column: Programs
    ImGui::Text("Programs (%zu)", uiState.allPrograms.size());
    ImGui::BeginChild("ProgramsList", ImVec2(0, 200), true);

    for (size_t i = 0; i < uiState.allPrograms.size(); i++) {
        const auto& prog = uiState.allPrograms[i];
        ImGui::PushID((int)i);

        std::string displayText = prog.title;

        // Add bank:PC info if available
        if (prog.bankPC.size() >= 3) {
            char buf[64];
            snprintf(buf, sizeof(buf), " [%u:%u:%u]", prog.bankPC[0], prog.bankPC[1], prog.bankPC[2]);
            displayText += buf;
        }

        if (ImGui::Selectable(displayText.c_str())) {
            // Send program change
            if (prog.bankPC.size() >= 3) {
                int channel = 0; // TODO: get channel from control or UI
                controller->sendProgramChange(channel, prog.bankPC[2], prog.bankPC[0], prog.bankPC[1]);
                std::cout << "Sent program change: " << displayText << std::endl;
            }
        }

        ImGui::PopID();
    }

    if (uiState.allPrograms.empty()) {
        ImGui::TextDisabled("No programs available");
    }

    ImGui::EndChild();

    ImGui::Columns(1);
}

void DrawLogWidget() {
    ImGui::SeparatorText("Message Log");

    ImGui::Checkbox("Auto-scroll", &uiState.autoScrollLogs);
    ImGui::SameLine();
    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(uiState.logMutex);
        uiState.logEntries.clear();
    }

    ImGui::BeginChild("LogScrollRegion", ImVec2(0, 300), true, ImGuiWindowFlags_HorizontalScrollbar);

    {
        std::lock_guard<std::mutex> lock(uiState.logMutex);
        for (const auto& entry : uiState.logEntries) {
            // Format timestamp
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                entry.timestamp.time_since_epoch()) % 1000;
            char time_buf[32];
            std::strftime(time_buf, sizeof(time_buf), "%H:%M:%S", std::localtime(&time_t));

            // Color based on direction
            ImVec4 color;
            const char* dir_str;
            if (entry.direction == midicci::keyboard::MessageDirection::In) {
                color = ImVec4(0.4f, 0.8f, 0.4f, 1.0f); // Green
                dir_str = "IN ";
            } else {
                color = ImVec4(0.8f, 0.8f, 0.4f, 1.0f); // Yellow
                dir_str = "OUT";
            }

            ImGui::PushStyleColor(ImGuiCol_Text, color);
            ImGui::Text("[%s.%03d] %s: %s", time_buf, (int)ms.count(), dir_str, entry.message.c_str());
            ImGui::PopStyleColor();
        }
    }

    if (uiState.autoScrollLogs && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
}

void DrawPianoKeyboard() {
    ImGui::Text("Piano Keyboard (Octave %d)", uiState.currentOctave);

    // Octave controls
    if (ImGui::Button("-")) {
        if (uiState.currentOctave > 0) uiState.currentOctave--;
    }
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        if (uiState.currentOctave < 10) uiState.currentOctave++;
    }
    ImGui::SameLine();
    ImGui::SliderInt("Velocity", &uiState.velocity, 1, 127);

    ImGui::Separator();

    // Draw piano keys
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float key_width = 40.0f;
    float white_key_height = 120.0f;
    float black_key_height = 80.0f;

    int white_key_index = 0;
    for (int i = 0; i < 12; i++) {
        bool is_black = (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);

        if (!is_black) {
            float x = canvas_pos.x + white_key_index * key_width;
            ImVec2 key_min(x, canvas_pos.y);
            ImVec2 key_max(x + key_width - 2, canvas_pos.y + white_key_height);

            ImU32 color = pianoState.keys[i] ? IM_COL32(100, 150, 255, 255) : IM_COL32(255, 255, 255, 255);
            draw_list->AddRectFilled(key_min, key_max, color);
            draw_list->AddRect(key_min, key_max, IM_COL32(0, 0, 0, 255));

            // Mouse interaction
            if (ImGui::IsMouseHoveringRect(key_min, key_max)) {
                if (ImGui::IsMouseClicked(0)) {
                    int note = uiState.currentOctave * 12 + i;
                    controller->noteOn(note, uiState.velocity);
                    pianoState.keys[i] = true;
                }
            }
            if (ImGui::IsMouseReleased(0) && pianoState.keys[i]) {
                int note = uiState.currentOctave * 12 + i;
                controller->noteOff(note);
                pianoState.keys[i] = false;
            }

            white_key_index++;
        }
    }

    // Draw black keys on top
    white_key_index = 0;
    for (int i = 0; i < 12; i++) {
        bool is_black = (i == 1 || i == 3 || i == 6 || i == 8 || i == 10);

        if (is_black) {
            int prev_white = (i == 1) ? 0 : (i == 3) ? 1 : (i == 6) ? 3 : (i == 8) ? 4 : 5;
            float x = canvas_pos.x + prev_white * key_width + key_width * 0.7f;
            ImVec2 key_min(x, canvas_pos.y);
            ImVec2 key_max(x + key_width * 0.6f, canvas_pos.y + black_key_height);

            ImU32 color = pianoState.keys[i] ? IM_COL32(50, 100, 200, 255) : IM_COL32(0, 0, 0, 255);
            draw_list->AddRectFilled(key_min, key_max, color);
            draw_list->AddRect(key_min, key_max, IM_COL32(100, 100, 100, 255));

            // Mouse interaction
            if (ImGui::IsMouseHoveringRect(key_min, key_max)) {
                if (ImGui::IsMouseClicked(0)) {
                    int note = uiState.currentOctave * 12 + i;
                    controller->noteOn(note, uiState.velocity);
                    pianoState.keys[i] = true;
                }
            }
            if (ImGui::IsMouseReleased(0) && pianoState.keys[i]) {
                int note = uiState.currentOctave * 12 + i;
                controller->noteOff(note);
                pianoState.keys[i] = false;
            }
        } else {
            white_key_index++;
        }
    }

    ImGui::Dummy(ImVec2(white_key_index * key_width, white_key_height + 10));
}

void DrawDeviceSelectors() {
    ImGui::SeparatorText("MIDI Devices");

    // Input device
    if (ImGui::BeginCombo("Input Device",
        uiState.selectedInputDevice >= 0 && uiState.selectedInputDevice < (int)uiState.inputDevices.size()
            ? uiState.inputDevices[uiState.selectedInputDevice].second.c_str()
            : "None")) {
        for (int i = 0; i < (int)uiState.inputDevices.size(); i++) {
            bool is_selected = (uiState.selectedInputDevice == i);
            if (ImGui::Selectable(uiState.inputDevices[i].second.c_str(), is_selected)) {
                uiState.selectedInputDevice = i;
                controller->selectInputDevice(uiState.inputDevices[i].first);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // Output device
    if (ImGui::BeginCombo("Output Device",
        uiState.selectedOutputDevice >= 0 && uiState.selectedOutputDevice < (int)uiState.outputDevices.size()
            ? uiState.outputDevices[uiState.selectedOutputDevice].second.c_str()
            : "None")) {
        for (int i = 0; i < (int)uiState.outputDevices.size(); i++) {
            bool is_selected = (uiState.selectedOutputDevice == i);
            if (ImGui::Selectable(uiState.outputDevices[i].second.c_str(), is_selected)) {
                uiState.selectedOutputDevice = i;
                controller->selectOutputDevice(uiState.outputDevices[i].first);
            }
            if (is_selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Refresh Devices")) {
        uiState.inputDevices = controller->getInputDevices();
        uiState.outputDevices = controller->getOutputDevices();
    }
}

void DrawMidiCIControls() {
    ImGui::SeparatorText("MIDI-CI");

    ImGui::Text("Status: %s", uiState.midiCIInitialized ? "Initialized" : "Not Initialized");
    if (uiState.midiCIInitialized) {
        ImGui::Text("MUID: 0x%08X", uiState.midiCIMuid);
        ImGui::Text("Device: %s", uiState.midiCIDeviceName.c_str());
    }

    if (ImGui::Button("Send Discovery")) {
        controller->sendMidiCIDiscovery();
        uiState.discoveredDevices = controller->getMidiCIDeviceDetails();
    }

    ImGui::Separator();
    ImGui::Text("Discovered Devices:");

    for (size_t i = 0; i < uiState.discoveredDevices.size(); i++) {
        const auto& dev = uiState.discoveredDevices[i];
        ImGui::PushID((int)i);
        if (ImGui::Selectable(dev.getDisplayName().c_str(), (int)i == uiState.selectedMidiCIDevice)) {
            uiState.selectedMidiCIDevice = (int)i;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("MUID: 0x%08X", dev.muid);
            ImGui::Text("Device: %s", dev.device_name.c_str());
            ImGui::Text("Manufacturer: %s", dev.manufacturer.c_str());
            ImGui::Text("Model: %s", dev.model.c_str());
            ImGui::Text("Version: %s", dev.version.c_str());
            ImGui::EndTooltip();
        }
        ImGui::PopID();
    }
}

int main(int argc, char** argv) {
    // Initialize logger
    logger = std::make_unique<midicci::keyboard::MessageLogger>();

    // Set up log callback
    logger->add_log_callback([](const midicci::keyboard::LogEntry& entry) {
        std::lock_guard<std::mutex> lock(uiState.logMutex);
        uiState.logEntries.push_back(entry);
        // Keep only last 1000 entries
        if (uiState.logEntries.size() > 1000) {
            uiState.logEntries.pop_front();
        }
    });

    // Initialize controller with logger
    controller = std::make_unique<KeyboardController>(logger.get());

    // Set up MIDI-CI callbacks
    controller->setMidiCIDevicesChangedCallback([&]() {
        std::cout << "MIDI-CI devices changed callback triggered" << std::endl;
        uiState.discoveredDevices = controller->getMidiCIDeviceDetails();
        std::cout << "Discovered " << uiState.discoveredDevices.size() << " MIDI-CI devices" << std::endl;
    });

    controller->setMidiConnectionChangedCallback([&](bool hasValidPair) {
        std::cout << "MIDI connection changed: " << (hasValidPair ? "valid pair" : "no pair") << std::endl;
        if (hasValidPair && controller->isMidiCIInitialized()) {
            std::cout << "Sending MIDI-CI Discovery automatically" << std::endl;
            controller->sendMidiCIDiscovery();
            uiState.discoveredDevices = controller->getMidiCIDeviceDetails();
        }
    });

    controller->setMidiCIPropertiesChangedCallback([&](uint32_t muid) {
        std::cout << "\n========================================" << std::endl;
        std::cout << "[CALLBACK] MIDI-CI properties changed for MUID: 0x" << std::hex << muid << std::dec << std::endl;
        std::cout << "========================================" << std::endl;

        // Auto-refresh properties if we have a selected device
        if (uiState.selectedMidiCIDevice >= 0 &&
            uiState.selectedMidiCIDevice < (int)uiState.discoveredDevices.size()) {
            uint32_t selectedMuid = uiState.discoveredDevices[uiState.selectedMidiCIDevice].muid;
            std::cout << "[CALLBACK] Selected device MUID: 0x" << std::hex << selectedMuid << std::dec << std::endl;

            if (selectedMuid == muid) {
                std::cout << "[CALLBACK] ✓ Matches selected device - auto-refreshing properties" << std::endl;

                auto controls_opt = controller->getAllCtrlList(selectedMuid);
                auto programs_opt = controller->getProgramList(selectedMuid);

                if (controls_opt.has_value()) {
                    std::cout << "[CALLBACK] ✓ Got " << controls_opt.value().size() << " controls" << std::endl;
                    uiState.allControls = controls_opt.value();
                } else {
                    std::cout << "[CALLBACK] ✗ No controls data" << std::endl;
                }

                if (programs_opt.has_value()) {
                    std::cout << "[CALLBACK] ✓ Got " << programs_opt.value().size() << " programs" << std::endl;
                    uiState.allPrograms = programs_opt.value();
                } else {
                    std::cout << "[CALLBACK] ✗ No programs data" << std::endl;
                }

                uiState.propertiesLoaded = true;
            } else {
                std::cout << "[CALLBACK] ✗ Does not match selected device - ignoring" << std::endl;
            }
        } else {
            std::cout << "[CALLBACK] ✗ No device selected - ignoring" << std::endl;
        }
        std::cout << "========================================\n" << std::endl;
    });

    // Update initial state
    uiState.inputDevices = controller->getInputDevices();
    uiState.outputDevices = controller->getOutputDevices();
    uiState.midiCIInitialized = controller->isMidiCIInitialized();
    uiState.midiCIMuid = controller->getMidiCIMuid();
    uiState.midiCIDeviceName = controller->getMidiCIDeviceName();

#ifdef IMGUI_BACKEND_SDL3
    // SDL3 Backend
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("MIDICCI Keyboard (ImGui)", 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);
            if (event.type == SDL_EVENT_QUIT) done = true;
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::Begin("MIDICCI Keyboard", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        DrawDeviceSelectors();
        ImGui::Separator();
        DrawPianoKeyboard();
        ImGui::Separator();
        DrawMidiCIControls();
        ImGui::Separator();
        DrawStandardProperties();
        ImGui::Separator();
        DrawLogWidget();

        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#elif defined(IMGUI_BACKEND_SDL2)
    // SDL2 Backend
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("MIDICCI Keyboard (ImGui)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 720, SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer2_Init(renderer);

    // Main loop
    bool done = false;
    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::Begin("MIDICCI Keyboard", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        DrawDeviceSelectors();
        ImGui::Separator();
        DrawPianoKeyboard();
        ImGui::Separator();
        DrawMidiCIControls();
        ImGui::Separator();
        DrawStandardProperties();
        ImGui::Separator();
        DrawLogWidget();

        ImGui::End();

        // Rendering
        ImGui::Render();
        SDL_SetRenderDrawColor(renderer, 45, 45, 48, 255);
        SDL_RenderClear(renderer);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());
        SDL_RenderPresent(renderer);
    }

    // Cleanup
    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

#elif defined(IMGUI_BACKEND_GLFW)
    // GLFW Backend
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "MIDICCI Keyboard (ImGui)", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Main window
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(io.DisplaySize, ImGuiCond_Always);
        ImGui::Begin("MIDICCI Keyboard", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        DrawDeviceSelectors();
        ImGui::Separator();
        DrawPianoKeyboard();
        ImGui::Separator();
        DrawMidiCIControls();
        ImGui::Separator();
        DrawStandardProperties();
        ImGui::Separator();
        DrawLogWidget();

        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.18f, 0.18f, 0.19f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif

    return 0;
}
