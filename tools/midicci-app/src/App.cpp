#include "App.hpp"

#include "imgui/SharedTheme.hpp"
#include <midicci/tooling/CIDeviceManager.hpp>
#include <midicci/tooling/MidiDeviceManager.hpp>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace midicci::app {

MidicciApplication::MidicciApplication() = default;

MidicciApplication::~MidicciApplication() {
    shutdown();
}

bool MidicciApplication::initialize() {
    if (initialized_) {
        return true;
    }
    SetupImGuiStyle();

    repository_ = std::make_unique<tooling::CIToolRepository>();

    log_callback_ = [this](const tooling::LogEntry& entry) {
        append_log_entry(entry);
    };
    repository_->add_log_callback(log_callback_);

    if (auto midi = repository_->get_midi_device_manager()) {
        midi->initialize();
    }
    if (auto ci = repository_->get_ci_device_manager()) {
        ci->initialize();
    }

    repository_->log("midicci-app initialized", tooling::MessageDirection::Out);
    keyboard_panel_ = std::make_unique<KeyboardPanel>(repository_.get());
    inspector_panel_ = std::make_unique<InspectorPanel>(repository_.get());
    local_device_panel_ = std::make_unique<LocalDevicePanel>(repository_.get());
    initialized_ = true;
    return true;
}

void MidicciApplication::shutdown() {
    if (!initialized_) {
        return;
    }

    if (repository_) {
        repository_->log("midicci-app shutting down", tooling::MessageDirection::Out);
        if (log_callback_) {
            repository_->remove_log_callback(log_callback_);
        }

        if (auto ci = repository_->get_ci_device_manager()) {
            ci->shutdown();
        }
        if (auto midi = repository_->get_midi_device_manager()) {
            midi->shutdown();
        }
    }

    local_device_panel_.reset();
    inspector_panel_.reset();
    keyboard_panel_.reset();
    repository_.reset();
    initialized_ = false;
}

bool MidicciApplication::render_frame() {
    if (!initialized_) {
        return false;
    }

    render_window();
    return true;
}

void MidicciApplication::render_window() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground;

    if (ImGui::Begin("midicci-app-root", nullptr, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 16.0f));

        if (ImGui::BeginTabBar("midicci-app-tabs")) {
            if (ImGui::BeginTabItem("Keyboard")) {
                ImGui::BeginChild("keyboard-scroll", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground);
                render_keyboard_tab();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Inspector")) {
                ImGui::BeginChild("inspector-scroll", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground);
                render_inspector_tab();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Local Device")) {
                ImGui::BeginChild("local-device-scroll", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground);
                render_local_device_tab();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Logs")) {
                ImGui::BeginChild("logs-scroll", ImVec2(0, 0), false, ImGuiWindowFlags_NoBackground);
                render_logs_tab();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::PopStyleVar();
    }
    ImGui::End();
}

void MidicciApplication::render_keyboard_tab() {
    if (keyboard_panel_) {
        keyboard_panel_->render();
    } else {
        ImGui::TextUnformatted("Keyboard panel not available.");
    }
}

void MidicciApplication::render_inspector_tab() {
    if (inspector_panel_) {
        inspector_panel_->render();
    } else {
        ImGui::TextUnformatted("Inspector panel unavailable.");
    }
}

void MidicciApplication::render_local_device_tab() {
    if (local_device_panel_) {
        local_device_panel_->render();
    } else {
        ImGui::TextUnformatted("Local device panel unavailable.");
    }
}

std::vector<MidicciApplication::LogLine> MidicciApplication::copy_logs_for_render() {
    std::lock_guard<std::mutex> lock(log_mutex_);
    return std::vector<LogLine>(log_lines_.begin(), log_lines_.end());
}

void MidicciApplication::render_logs_tab() {
    auto entries = copy_logs_for_render();
    ImGui::Text("Entries: %zu", entries.size());

    if (ImGui::Button("Clear")) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_lines_.clear();
    }
    ImGui::SameLine();
    log_filter_.Draw("Filter");
    ImGui::SameLine();
    ImGui::Checkbox("Auto-scroll", &auto_scroll_logs_);

    ImGui::BeginChild("midicci-log-scroll", ImVec2(0, 0), true,
                      ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);

    for (size_t idx = 0; idx < entries.size(); ++idx) {
        const auto& entry = entries[idx];
        if (!log_filter_.PassFilter(entry.message.c_str())) {
            continue;
        }

        ImVec4 color = entry.direction == tooling::MessageDirection::In
                           ? ImVec4(0.4f, 0.75f, 1.0f, 1.0f)
                           : ImVec4(0.6f, 1.0f, 0.6f, 1.0f);

        std::string timestamp = format_timestamp(entry.timestamp);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        ImGui::Text("[%s] %s (src=0x%08X dst=0x%08X)",
                    timestamp.c_str(),
                    entry.direction == tooling::MessageDirection::In ? "IN" : "OUT",
                    entry.source_muid,
                    entry.destination_muid);
        ImGui::PopStyleColor();

        ImGui::PushID(static_cast<int>(idx));
        ImGui::TextWrapped("%s", entry.message.c_str());
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            ImGui::SetClipboardText(entry.message.c_str());
        }
        if (ImGui::BeginPopupContextItem("log_context")) {
            if (ImGui::MenuItem("Copy")) {
                ImGui::SetClipboardText(entry.message.c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::PopID();
        ImGui::Separator();
    }

    if (auto_scroll_logs_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
}

void MidicciApplication::append_log_entry(const tooling::LogEntry& entry) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    log_lines_.push_back(LogLine{
        entry.timestamp,
        entry.direction,
        entry.source_muid,
        entry.destination_muid,
        entry.message
    });
    if (log_lines_.size() > kMaxLogLines) {
        log_lines_.pop_front();
    }
}

std::string MidicciApplication::format_timestamp(const std::chrono::system_clock::time_point& ts) const {
    auto tt = std::chrono::system_clock::to_time_t(ts);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");
    return oss.str();
}

} // namespace midicci::app
