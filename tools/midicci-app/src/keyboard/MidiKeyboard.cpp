#include "MidiKeyboard.hpp"

#include <algorithm>
#include <cstdio>

namespace midicci::app {

MidiKeyboard::MidiKeyboard() {
    pressed_keys_.resize(128, false);
    external_pressed_keys_.resize(128, false);
    setup_keys();
}

void MidiKeyboard::set_octave_range(int start_octave, int num_octaves) {
    num_octaves_ = std::clamp(num_octaves, 1, 10);
    const int max_start = std::max(0, 10 - num_octaves_);
    octave_start_ = std::clamp(start_octave, 0, max_start);
    setup_keys();
}

void MidiKeyboard::set_key_size(float width, float white_height, float black_height) {
    key_width_ = width;
    white_key_height_ = white_height;
    black_key_height_ = black_height;
    setup_keys();
}

void MidiKeyboard::set_key_event_callback(std::function<void(int note, int velocity, bool is_pressed)> callback) {
    on_key_event_ = std::move(callback);
}

void MidiKeyboard::shift_octave(int delta) {
    if (delta == 0) {
        return;
    }
    release_all_keys();
    const int max_start = std::max(0, 10 - num_octaves_);
    octave_start_ = std::clamp(octave_start_ + delta, 0, max_start);
    setup_keys();
}

void MidiKeyboard::setup_keys() {
    keys_.clear();

    float current_x = 0.0f;
    int start_note = octave_start_ * 12;
    int end_note = start_note + (num_octaves_ * 12);

    for (int note = start_note; note < end_note; ++note) {
        int note_in_octave = note % 12;
        bool is_black = is_black_key(note_in_octave);

        if (!is_black) {
            keys_.push_back({note, false, current_x, key_width_});
            current_x += key_width_;
        }
    }

    current_x = 0.0f;
    for (int note = start_note; note < end_note; ++note) {
        int note_in_octave = note % 12;
        bool is_black = is_black_key(note_in_octave);

        if (!is_black) {
            current_x += key_width_;
        } else {
            float black_key_x = current_x - key_width_ * 0.3f;
            keys_.push_back({note, true, black_key_x, key_width_ * 0.6f});
        }
    }

    std::sort(keys_.begin(), keys_.end(), [](const KeyInfo& a, const KeyInfo& b) {
        return !a.is_black && b.is_black;
    });
}

void MidiKeyboard::render() {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
    float keyboard_width = (num_octaves_ * 7) * key_width_;
    float side_button_width = key_width_;
    float total_width = keyboard_width + (side_button_width * 2.0f);
    ImVec2 canvas_size = ImVec2(total_width, white_key_height_);

    ImGui::InvisibleButton("##keyboard", canvas_size);

    bool is_hovered = ImGui::IsItemHovered();
    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool mouse_down = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool mouse_clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
    float relative_x_full = mouse_pos.x - canvas_pos.x;
    float relative_y = mouse_pos.y - canvas_pos.y;
    float keyboard_start_x = canvas_pos.x + side_button_width;
    float relative_x = mouse_pos.x - keyboard_start_x;

    bool in_left_button = relative_x_full >= 0.0f && relative_x_full < side_button_width &&
                          relative_y >= 0.0f && relative_y < white_key_height_;
    bool in_right_button = relative_x_full >= (side_button_width + keyboard_width) &&
                           relative_x_full < total_width &&
                           relative_y >= 0.0f && relative_y < white_key_height_;
    bool left_hovered = is_hovered && in_left_button;
    bool right_hovered = is_hovered && in_right_button;

    if (is_hovered && mouse_clicked) {
        if (left_hovered) {
            shift_octave(-1);
        } else if (right_hovered) {
            shift_octave(1);
        }
    }

    if (is_hovered && relative_x >= 0.0f && relative_x < keyboard_width &&
        relative_y >= 0.0f && relative_y < white_key_height_) {
        int hovered_note = get_note_from_position(relative_x, relative_y);

        if (mouse_down && hovered_note != -1) {
            if (mouse_down_key_ != hovered_note) {
                if (mouse_down_key_ != -1) {
                    release_key(mouse_down_key_);
                }
                press_key(hovered_note);
                mouse_down_key_ = hovered_note;
            }
        } else if (!mouse_down && mouse_down_key_ != -1) {
            release_key(mouse_down_key_);
            mouse_down_key_ = -1;
        }
    } else if (!mouse_down && mouse_down_key_ != -1) {
        release_key(mouse_down_key_);
        mouse_down_key_ = -1;
    }

    auto draw_shift_button = [&](ImVec2 pos, const char* glyph, bool hovered) {
        ImVec2 size(side_button_width, white_key_height_);
        ImU32 key_color = hovered ? IM_COL32(210, 210, 230, 255) : IM_COL32(235, 235, 235, 255);
        ImU32 border_color = IM_COL32(100, 100, 100, 255);
        draw_list->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), key_color);
        draw_list->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), border_color);

        ImVec2 text_size = ImGui::CalcTextSize(glyph);
        ImVec2 text_pos = ImVec2(pos.x + (size.x - text_size.x) * 0.5f,
                                 pos.y + (size.y - text_size.y) * 0.5f);
        draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), glyph);
    };

    draw_shift_button(canvas_pos, "<", left_hovered);
    draw_shift_button(ImVec2(canvas_pos.x + side_button_width + keyboard_width, canvas_pos.y), ">",
                      right_hovered);

    for (const auto& key : keys_) {
        ImVec2 key_pos = ImVec2(keyboard_start_x + key.x, canvas_pos.y);
        ImVec2 key_size = ImVec2(key.width, key.is_black ? black_key_height_ : white_key_height_);

        bool is_pressed = pressed_keys_[key.note] || external_pressed_keys_[key.note];
        bool is_highlighted = (highlighted_key_ == key.note);
        ImU32 key_color;
        ImU32 border_color = IM_COL32(100, 100, 100, 255);

        auto blend_color = [](ImU32 base, ImU32 overlay) {
            ImVec4 base_color = ImGui::ColorConvertU32ToFloat4(base);
            ImVec4 overlay_color = ImGui::ColorConvertU32ToFloat4(overlay);
            ImVec4 result = {
                overlay_color.x * overlay_color.w + base_color.x * (1.0f - overlay_color.w),
                overlay_color.y * overlay_color.w + base_color.y * (1.0f - overlay_color.w),
                overlay_color.z * overlay_color.w + base_color.z * (1.0f - overlay_color.w),
                1.0f
            };
            return ImGui::ColorConvertFloat4ToU32(result);
        };

        if (key.is_black) {
            key_color = is_pressed ? IM_COL32(100, 100, 100, 255) : IM_COL32(50, 50, 50, 255);
        } else {
            key_color = is_pressed ? IM_COL32(200, 200, 255, 255) : IM_COL32(255, 255, 255, 255);
        }

        if (is_highlighted && !is_pressed) {
            const ImU32 highlight_overlay = key.is_black ? IM_COL32(120, 80, 180, 180)
                                                         : IM_COL32(120, 160, 255, 200);
            key_color = blend_color(key_color, highlight_overlay);
        }

        draw_list->AddRectFilled(key_pos, ImVec2(key_pos.x + key_size.x, key_pos.y + key_size.y), key_color);
        draw_list->AddRect(key_pos, ImVec2(key_pos.x + key_size.x, key_pos.y + key_size.y), border_color);

        if (!key.is_black && (key.note % 12) == 0) {
            const char* note_name = get_note_name(key.note);
            ImVec2 text_size = ImGui::CalcTextSize(note_name);
            ImVec2 text_pos = ImVec2(
                key_pos.x + (key_size.x - text_size.x) * 0.5f,
                key_pos.y + key_size.y - text_size.y - 5.0f
            );
            draw_list->AddText(text_pos, IM_COL32(0, 0, 0, 255), note_name);
        }
    }
}

void MidiKeyboard::press_key(int note, int velocity) {
    if (note < 0 || note >= static_cast<int>(pressed_keys_.size())) {
        return;
    }
    if (!pressed_keys_[note]) {
        pressed_keys_[note] = true;
        handle_key_event(note, true, velocity);
    }
}

void MidiKeyboard::release_key(int note) {
    if (note < 0 || note >= static_cast<int>(pressed_keys_.size())) {
        return;
    }
    if (pressed_keys_[note]) {
        pressed_keys_[note] = false;
        handle_key_event(note, false, 0);
    }
}

void MidiKeyboard::release_all_keys() {
    for (int note = 0; note < static_cast<int>(pressed_keys_.size()); ++note) {
        if (pressed_keys_[note]) {
            release_key(note);
        }
    }
}

void MidiKeyboard::set_highlighted_key(int note) {
    highlighted_key_ = note;
}

void MidiKeyboard::set_external_key_state(int note, bool is_pressed) {
    if (note < 0 || note >= static_cast<int>(external_pressed_keys_.size())) {
        return;
    }
    external_pressed_keys_[note] = is_pressed;
}

int MidiKeyboard::get_note_from_position(float x, float y) {
    int selected_note = -1;
    float white_key_width = key_width_;

    for (const auto& key : keys_) {
        if (!key.is_black) {
            float key_start = key.x;
            float key_end = key.x + white_key_width;
            if (x >= key_start && x < key_end) {
                selected_note = key.note;
                break;
            }
        }
    }

    for (const auto& key : keys_) {
        if (key.is_black) {
            float key_start = key.x;
            float key_end = key.x + key.width;
            if (x >= key_start && x < key_end && y >= 0.0f && y < black_key_height_) {
                selected_note = key.note;
                break;
            }
        }
    }

    return selected_note;
}

bool MidiKeyboard::is_black_key(int note) {
    switch (note % 12) {
        case 1:
        case 3:
        case 6:
        case 8:
        case 10:
            return true;
        default:
            return false;
    }
}

const char* MidiKeyboard::get_note_name(int note) {
    static const char* note_names[] = {
        "C", "C#", "D", "D#", "E", "F",
        "F#", "G", "G#", "A", "A#", "B"
    };
    static char name_buffer[8];
    int octave = note / 12;
    int note_in_octave = note % 12;
    snprintf(name_buffer, sizeof(name_buffer), "%s%d", note_names[note_in_octave], octave);
    return name_buffer;
}

void MidiKeyboard::handle_key_event(int note, bool is_pressed, int velocity) {
    if (on_key_event_) {
        on_key_event_(note, velocity, is_pressed);
    }
}

} // namespace midicci::app
