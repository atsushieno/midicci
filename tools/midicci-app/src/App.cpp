#include "App.hpp"

#include "EmbeddedFont.hpp"
#include <midicci/tooling/CIDeviceManager.hpp>
#include <midicci/tooling/MidiDeviceManager.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <vector>
#include <zlib.h>

namespace {

std::vector<uint8_t> extract_ttf_from_zip(const uint8_t* zip_data, size_t zip_size) {
    constexpr uint32_t kEndOfCentralDirSig = 0x06054b50;
    constexpr uint32_t kCentralDirSig = 0x02014b50;
    constexpr uint32_t kLocalFileHeaderSig = 0x04034b50;

    if (zip_size < 22) {
        return {};
    }

    size_t eocd_offset = zip_size - 22;
    while (eocd_offset > 0) {
        if (*reinterpret_cast<const uint32_t*>(zip_data + eocd_offset) == kEndOfCentralDirSig) {
            break;
        }
        eocd_offset--;
    }

    if (eocd_offset == 0) {
        return {};
    }

    uint32_t central_dir_offset = *reinterpret_cast<const uint32_t*>(zip_data + eocd_offset + 16);
    if (central_dir_offset + 46 > zip_size) {
        return {};
    }

    const uint8_t* central_header = zip_data + central_dir_offset;
    if (*reinterpret_cast<const uint32_t*>(central_header) != kCentralDirSig) {
        return {};
    }

    uint16_t compression = *reinterpret_cast<const uint16_t*>(central_header + 10);
    uint32_t compressed_size = *reinterpret_cast<const uint32_t*>(central_header + 20);
    uint32_t uncompressed_size = *reinterpret_cast<const uint32_t*>(central_header + 24);
    uint32_t local_header_offset = *reinterpret_cast<const uint32_t*>(central_header + 42);

    if (local_header_offset + 30 > zip_size) {
        return {};
    }

    const uint8_t* local_header = zip_data + local_header_offset;
    if (*reinterpret_cast<const uint32_t*>(local_header) != kLocalFileHeaderSig) {
        return {};
    }

    uint16_t local_name_len = *reinterpret_cast<const uint16_t*>(local_header + 26);
    uint16_t local_extra_len = *reinterpret_cast<const uint16_t*>(local_header + 28);

    size_t data_offset = local_header_offset + 30 + local_name_len + local_extra_len;
    if (data_offset + compressed_size > zip_size) {
        return {};
    }

    const uint8_t* compressed_data = zip_data + data_offset;

    if (compression == 0) {
        return std::vector<uint8_t>(compressed_data, compressed_data + uncompressed_size);
    } else if (compression == 8) {
        std::vector<uint8_t> result(uncompressed_size);
        z_stream stream{};
        stream.next_in = const_cast<uint8_t*>(compressed_data);
        stream.avail_in = compressed_size;
        stream.next_out = result.data();
        stream.avail_out = uncompressed_size;

        if (inflateInit2(&stream, -15) != Z_OK) {
            return {};
        }

        int ret = inflate(&stream, Z_FINISH);
        inflateEnd(&stream);

        if (ret == Z_STREAM_END) {
            return result;
        }
    }

    return {};
}

void ensure_application_font() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    io.FontDefault = nullptr;

    static std::vector<uint8_t> ttf_data = extract_ttf_from_zip(
        midicci::app::kEmbeddedFontData,
        midicci::app::kEmbeddedFontSize
    );
    if (!ttf_data.empty()) {
        constexpr float kBaseFontSize = 16.0f;
        ImFontConfig config;
        config.OversampleH = 2;
        config.OversampleV = 1;
        config.PixelSnapH = false;
        config.FontDataOwnedByAtlas = false;
        ImFont* font = io.Fonts->AddFontFromMemoryTTF(
            ttf_data.data(),
            static_cast<int>(ttf_data.size()),
            kBaseFontSize,
            &config
        );
        if (font != nullptr) {
            io.FontDefault = font;
            return;
        }
    }

    std::fprintf(stderr, "midicci-app: failed to load embedded font\n");
    ImFont* fallback = io.Fonts->AddFontDefault();
    io.FontDefault = fallback;
}

} // namespace

namespace midicci::app {

MidicciApplication::MidicciApplication() = default;

MidicciApplication::~MidicciApplication() {
    shutdown();
}

bool MidicciApplication::initialize() {
    if (initialized_) {
        return true;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr; // disable ImGui's built-in ini persistence
    ImGui::LoadIniSettingsFromMemory("", 0);

    ensure_application_font();
    apply_theme(theme_mode_);
    ui_scale_dirty_ = false;

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
    ui_scale_dirty_ = false;
    return true;
}

void MidicciApplication::render_window() {
    update_window_size_tracking();

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus |
                             ImGuiWindowFlags_NoBackground;
    if (theme_mode_ == ThemeMode::Light) {
        flags &= ~ImGuiWindowFlags_NoBackground;
    }

    if (ImGui::Begin("midicci-app-root", nullptr, flags)) {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f * ui_scale_, 16.0f * ui_scale_));

        render_scale_toolbar();
        ImGui::Separator();
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

void MidicciApplication::render_scale_toolbar() {
    ImGui::BeginGroup();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Scale:");
    ImGui::SameLine();

    static constexpr float kScaleOptions[] = {0.5f, 0.8f, 1.0f, 1.25f, 1.5f, 2.0f, 3.0f};
    static constexpr const char* kScaleLabels[] = {"x0.5", "x0.8", "x1.0", "x1.25", "x1.5", "x2.0", "x3.0"};
    constexpr int kScaleCount = sizeof(kScaleOptions) / sizeof(kScaleOptions[0]);

    int currentIndex = 0;
    for (int i = 0; i < kScaleCount; ++i) {
        if (std::fabs(ui_scale_ - kScaleOptions[i]) < 0.001f) {
            currentIndex = i;
            break;
        }
    }
    int selectedIndex = currentIndex;

    ImGui::SetNextItemWidth(120.0f * ui_scale_);
    if (ImGui::BeginCombo("##midicci-scale", kScaleLabels[currentIndex])) {
        for (int i = 0; i < kScaleCount; ++i) {
            bool isSelected = (selectedIndex == i);
            if (ImGui::Selectable(kScaleLabels[i], isSelected)) {
                selectedIndex = i;
            }
            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    if (selectedIndex != currentIndex) {
        apply_ui_scale(kScaleOptions[selectedIndex]);
        request_window_resize();
    }
    ImGui::SameLine();
    const char* themeLabel = (theme_mode_ == ThemeMode::Dark) ? "> Light" : "> Dark";
    if (ImGui::Button(themeLabel)) {
        toggle_theme();
    }
    ImGui::EndGroup();
    ImGui::Spacing();
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

    if (ImGui::Button("Clear Logs")) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        log_lines_.clear();
    }
    ImGui::SameLine();
    log_filter_.Draw("Filter");
    ImGui::SameLine();
    if (ImGui::Button("Reset Filter")) {
        log_filter_.Clear();
    }
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
                           : ImVec4(0.35f, 0.55f, 0.35f, 1.0f);

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

bool MidicciApplication::consume_pending_window_resize(ImVec2& size) {
    if (!window_size_request_pending_) {
        return false;
    }
    size = requested_window_size_;
    window_size_request_pending_ = false;
    return true;
}

void MidicciApplication::apply_ui_scale(float scale) {
    ui_scale_ = std::clamp(scale, 0.5f, 4.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style = base_style_;
    style.ScaleAllSizes(ui_scale_);

    apply_font_scaling();
    ui_scale_dirty_ = true;
}

void MidicciApplication::capture_font_scales() {
    ImGuiIO& io = ImGui::GetIO();
    base_font_scales_.clear();
    base_font_scales_.reserve(static_cast<size_t>(io.Fonts->Fonts.Size));
    for (int i = 0; i < io.Fonts->Fonts.Size; ++i) {
        base_font_scales_.push_back(io.Fonts->Fonts[i]->Scale);
    }
    font_scales_captured_ = true;
}

void MidicciApplication::apply_font_scaling() {
    ImGuiIO& io = ImGui::GetIO();
    if (!font_scales_captured_ || base_font_scales_.size() != static_cast<size_t>(io.Fonts->Fonts.Size)) {
        capture_font_scales();
    }
    for (int i = 0; i < io.Fonts->Fonts.Size; ++i) {
        io.Fonts->Fonts[i]->Scale = base_font_scales_[i] * ui_scale_;
    }
    io.FontGlobalScale = 1.0f;
}

void MidicciApplication::request_window_resize() {
    ImGuiIO& io = ImGui::GetIO();
    if (base_window_size_.x <= 0.0f || base_window_size_.y <= 0.0f) {
        if (io.DisplaySize.x > 0.0f && io.DisplaySize.y > 0.0f) {
            float safeScale = std::max(ui_scale_, 0.001f);
            base_window_size_.x = io.DisplaySize.x / safeScale;
            base_window_size_.y = io.DisplaySize.y / safeScale;
        }
    }

    requested_window_size_.x = std::max(200.0f, base_window_size_.x * ui_scale_);
    requested_window_size_.y = std::max(200.0f, base_window_size_.y * ui_scale_);
    window_size_request_pending_ = true;
    waiting_for_window_resize_ = true;
}

void MidicciApplication::update_window_size_tracking() {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 displaySize = io.DisplaySize;
    if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) {
        return;
    }

    constexpr float kWindowSizeEpsilon = 1.0f;
    if (last_window_size_.x == 0.0f && last_window_size_.y == 0.0f) {
        float safeScale = std::max(ui_scale_, 0.001f);
        base_window_size_.x = displaySize.x / safeScale;
        base_window_size_.y = displaySize.y / safeScale;
    }

    float deltaX = std::fabs(displaySize.x - last_window_size_.x);
    float deltaY = std::fabs(displaySize.y - last_window_size_.y);

    if (waiting_for_window_resize_) {
        bool reachedTarget = std::fabs(displaySize.x - requested_window_size_.x) < kWindowSizeEpsilon &&
                             std::fabs(displaySize.y - requested_window_size_.y) < kWindowSizeEpsilon;
        if (reachedTarget || deltaX > kWindowSizeEpsilon || deltaY > kWindowSizeEpsilon) {
            waiting_for_window_resize_ = false;
            if (ui_scale_ > 0.0f) {
                base_window_size_.x = displaySize.x / ui_scale_;
                base_window_size_.y = displaySize.y / ui_scale_;
            }
        }
    } else if (deltaX > kWindowSizeEpsilon || deltaY > kWindowSizeEpsilon) {
        if (ui_scale_ > 0.0f) {
            base_window_size_.x = displaySize.x / ui_scale_;
            base_window_size_.y = displaySize.y / ui_scale_;
        }
    }

    last_window_size_ = displaySize;
}

void MidicciApplication::toggle_theme() {
    const ThemeMode nextMode = (theme_mode_ == ThemeMode::Dark) ? ThemeMode::Light : ThemeMode::Dark;
    apply_theme(nextMode);
}

void MidicciApplication::apply_theme(ThemeMode mode) {
    theme_mode_ = mode;
    SetupImGuiStyle(mode);
    base_style_ = ImGui::GetStyle();
    apply_ui_scale(ui_scale_);
}

} // namespace midicci::app
