#pragma once

#include "imgui/ImGuiEventLoop.hpp"
#include "keyboard/KeyboardPanel.hpp"
#include "inspector/InspectorPanel.hpp"
#include "local_device/LocalDevicePanel.hpp"
#include <midicci/tooling/CIToolRepository.hpp>
#include <imgui.h>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace midicci::app {

class MidicciApplication {
public:
    MidicciApplication();
    ~MidicciApplication();

    bool initialize();
    void shutdown();
    bool render_frame();

    tooling::CIToolRepository* repository() { return repository_.get(); }

private:
    struct LogLine {
        std::chrono::system_clock::time_point timestamp;
        tooling::MessageDirection direction;
        uint32_t source_muid;
        uint32_t destination_muid;
        std::string message;
    };

    std::vector<LogLine> copy_logs_for_render();
    void append_log_entry(const tooling::LogEntry& entry);
    void render_window();
    void render_keyboard_tab();
    void render_inspector_tab();
    void render_local_device_tab();
    void render_logs_tab();
    std::string format_timestamp(const std::chrono::system_clock::time_point& ts) const;

    std::unique_ptr<tooling::CIToolRepository> repository_;
    std::function<void(const tooling::LogEntry&)> log_callback_;

    std::mutex log_mutex_;
    std::deque<LogLine> log_lines_;
    static constexpr size_t kMaxLogLines = 2000;

    ImGuiTextFilter log_filter_;
    bool auto_scroll_logs_ = true;
    bool initialized_ = false;

    std::unique_ptr<KeyboardPanel> keyboard_panel_;
    std::unique_ptr<InspectorPanel> inspector_panel_;
    std::unique_ptr<LocalDevicePanel> local_device_panel_;
};

} // namespace midicci::app
