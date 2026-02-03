#pragma once

#include <imgui.h>
#include <functional>
#include <vector>

namespace midicci::app {

class MidiKeyboard {
public:
    struct KeyPress {
        int note;
        int velocity;
        bool is_pressed;
    };

    MidiKeyboard();

    void set_octave_range(int start_octave, int num_octaves);
    void set_key_size(float width, float white_height, float black_height);
    void set_key_event_callback(std::function<void(int note, int velocity, bool is_pressed)> callback);
    void shift_octave(int delta);
    int octave_start() const { return octave_start_; }
    int num_octaves() const { return num_octaves_; }

    void render();
    void press_key(int note, int velocity = 100);
    void release_key(int note);
    void release_all_keys();
    void set_highlighted_key(int note);
    void set_external_key_state(int note, bool is_pressed);

private:
    struct KeyInfo {
        int note;
        bool is_black;
        float x;
        float width;
    };

    int octave_start_ = 5;
    int num_octaves_ = 2;
    float key_width_ = 24.0f;
    float white_key_height_ = 100.0f;
    float black_key_height_ = 60.0f;

    std::vector<bool> pressed_keys_;
    std::vector<bool> external_pressed_keys_;
    std::vector<KeyInfo> keys_;
    int mouse_down_key_ = -1;
    int highlighted_key_ = -1;

    std::function<void(int note, int velocity, bool is_pressed)> on_key_event_;

    void setup_keys();
    int get_note_from_position(float x, float y);
    bool is_black_key(int note);
    const char* get_note_name(int note);
    void handle_key_event(int note, bool is_pressed, int velocity = 100);
};

} // namespace midicci::app
