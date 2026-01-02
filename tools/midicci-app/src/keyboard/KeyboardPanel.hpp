#pragma once

#include "MidiKeyboard.hpp"
#include <keyboard_controller.h>
#include <midi_ci_manager.h>
#include <message_logger.h>
#include <midicci/tooling/CIToolRepository.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>

namespace midicci::app {

class KeyboardPanel {
public:
    explicit KeyboardPanel(tooling::CIToolRepository* repository);
    ~KeyboardPanel();

    void render();

private:
    struct DeviceEntry {
        std::string id;
        std::string name;
    };

    enum class ParameterContext {
        Global = 0,
        Group,
        Channel,
        Key
    };

    void apply_pending_updates();
    void refresh_devices();
    void refresh_ci_devices();
    void render_transport_section();
    void render_keyboard_section();
    void render_ci_section();
    void render_selected_ci_device();
    void render_ci_property_tools(uint32_t muid);
    uint32_t current_selected_muid() const;
    void select_input_device(int index);
    void select_output_device(int index);

    midicci::keyboard::MessageLogger message_logger_;
    std::unique_ptr<KeyboardController> controller_;
    MidiKeyboard midi_keyboard_;
    MidiKeyboard parameter_keyboard_;

    std::vector<DeviceEntry> input_devices_;
    std::vector<DeviceEntry> output_devices_;
    int selected_input_index_ = -1;
    int selected_output_index_ = -1;
    std::string current_input_id_;
    std::string current_output_id_;

    std::vector<MidiCIDeviceInfo> ci_devices_;
    int selected_ci_index_ = -1;

    int velocity_value_ = 100;
    std::atomic<bool> devices_dirty_{true};
    std::atomic<bool> ci_dirty_{true};
    std::mutex state_mutex_;

    tooling::CIToolRepository* repository_ = nullptr;
    midicci::keyboard::MessageLogger::LogCallback log_bridge_;
    uint32_t last_selected_muid_ = 0;

    struct ControlMapCache {
        std::vector<midicci::commonproperties::MidiCIControlMap> values;
        bool pending = false;
        bool loaded = false;
        bool checked_local = false;
        int last_visible_frame = -1;
        std::chrono::steady_clock::time_point last_request_time{};
    };

    struct PendingPropertyUpdate {
        uint32_t muid;
        std::string property_id;
        std::string res_id;
    };

    void process_property_updates();
    void invalidate_invisible_ctrl_map_entries(uint32_t muid, int current_frame);

    std::unordered_map<uint32_t, std::unordered_map<std::string, ControlMapCache>> ctrl_map_cache_;
    std::unordered_map<uint32_t, std::vector<midicci::commonproperties::MidiCIControl>> ctrl_list_cache_;
    std::unordered_map<uint32_t, std::vector<midicci::commonproperties::MidiCIProgram>> program_list_cache_;
    std::unordered_map<uint32_t, int> selected_program_index_;
    std::mutex property_update_mutex_;
    std::vector<PendingPropertyUpdate> pending_property_updates_;

    std::unordered_map<std::string, int> control_values_;

    ParameterContext parameter_context_ = ParameterContext::Global;
    int parameter_group_value_ = 0;
    int parameter_channel_value_ = 0;
    int parameter_key_value_ = 60;

    void render_parameter_context_controls();
    int current_group_value() const;
    int current_channel_value() const;
    int current_key_value() const;
    int resolve_channel(const midicci::commonproperties::MidiCIControl& ctrl) const;
    void send_control_value(const midicci::commonproperties::MidiCIControl& ctrl, uint32_t value);
    void set_parameter_key_value(int note);
    bool control_matches_context(const midicci::commonproperties::MidiCIControl& ctrl) const;
};

} // namespace midicci::app
