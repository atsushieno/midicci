#include "KeyboardPanel.hpp"

#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <iomanip>
#include <limits>
#include <sstream>
#include <fstream>
#include <portable-file-dialogs.h>
#include <memory>
#include <midicci/details/commonproperties/StandardProperties.hpp>
#include <cctype>

namespace midicci::app {

namespace {
int convert_velocity_to_16bit(int value7) {
    int clamped = std::clamp(value7, 1, 127);
    return clamped << 9;
}

constexpr auto CTRL_MAP_REQUEST_TIMEOUT = std::chrono::seconds(3);
constexpr const char* PARAM_CONTEXT_LABELS[] = {"Global", "Group", "Channel", "Key"};
constexpr int PARAM_CONTEXT_COUNT = static_cast<int>(sizeof(PARAM_CONTEXT_LABELS) / sizeof(PARAM_CONTEXT_LABELS[0]));

std::string note_label(int note) {
    static constexpr const char* NOTE_NAMES[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    if (note < 0 || note > 127) {
        return "N/A";
    }
    int octave = (note / 12) - 1;
    std::ostringstream oss;
    oss << NOTE_NAMES[note % 12] << octave;
    return oss.str();
}

std::string format_parameter_id(const midicci::commonproperties::MidiCIControl& ctrl) {
    if (ctrl.ctrlIndex.empty()) {
        return "-";
    }
    const size_t bytes = std::min<size_t>(ctrl.ctrlIndex.size(), 4);
    uint32_t combined = 0;
    for (size_t i = 0; i < bytes; ++i) {
        combined = (combined << 8) | static_cast<uint32_t>(ctrl.ctrlIndex[i]);
    }
    std::ostringstream oss;
    oss << "0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(static_cast<int>(bytes * 2))
        << combined;
    if (ctrl.ctrlIndex.size() > bytes) {
        for (size_t i = bytes; i < ctrl.ctrlIndex.size(); ++i) {
            oss << '/' << static_cast<int>(ctrl.ctrlIndex[i]);
        }
    }
    return oss.str();
}

std::string format_parameter_path(const midicci::commonproperties::MidiCIControl& ctrl) {
    if (ctrl.paramPath.has_value() && !ctrl.paramPath.value().empty()) {
        return ctrl.paramPath.value();
    }
    return "-";
}
} // namespace

KeyboardPanel::KeyboardPanel(tooling::CIToolRepository* repository)
    : repository_(repository) {
    if (repository_) {
        log_bridge_ = [this](const midicci::keyboard::LogEntry& entry) {
            auto direction = entry.direction == midicci::keyboard::MessageDirection::In
                                 ? tooling::MessageDirection::In
                                 : tooling::MessageDirection::Out;
            repository_->log(entry.message, direction, entry.source_muid, entry.destination_muid);
        };
        message_logger_.add_log_callback(log_bridge_);
    }

    note_callback_active_ = std::make_shared<std::atomic_bool>(true);

    midi_keyboard_.set_octave_range(2, 4);
    controller_ = std::make_unique<KeyboardController>(&message_logger_);
    midi_keyboard_.set_key_event_callback([this](int note, int /*velocity*/, bool is_pressed) {
        if (!controller_) {
            return;
        }
        if (is_pressed) {
            controller_->noteOn(note, convert_velocity_to_16bit(velocity_value_));
        } else {
            controller_->noteOff(note);
        }
    });

    parameter_keyboard_.set_octave_range(4, 2);
    parameter_keyboard_.set_key_size(16.0f, 48.0f, 32.0f);
    parameter_keyboard_.set_key_event_callback([this](int note, int /*velocity*/, bool is_pressed) {
        if (is_pressed) {
            set_parameter_key_value(note);
        }
    });
    set_parameter_key_value(parameter_key_value_);

    controller_->setIncomingNoteCallback([this](int note, int velocity, bool is_pressed) {
        enqueue_incoming_note_event(note, velocity, is_pressed);
    });

    controller_->setIncomingControlValueCallback([this](const KeyboardController::IncomingControlValue& value) {
        enqueue_incoming_control_event(value);
    });

    if (repository_) {
        if (auto midi_manager = repository_->get_midi_device_manager()) {
            std::weak_ptr<std::atomic_bool> weak_flag = note_callback_active_;
            midi_manager->add_note_event_callback([this, weak_flag](int note, int velocity, bool is_pressed) {
                auto flag = weak_flag.lock();
                if (!flag || !flag->load()) {
                    return;
                }
                enqueue_incoming_note_event(note, velocity, is_pressed);
            });
            std::weak_ptr<tooling::MidiDeviceManager> weak_manager = midi_manager;
            controller_->setExternalOutputCallback([weak_manager](const libremidi::ump& packet) {
                if (auto manager = weak_manager.lock()) {
                    manager->send_to_virtual_output(packet);
                }
            });
        }
    }

    controller_->setMidiCIDevicesChangedCallback([this]() {
        ci_dirty_.store(true, std::memory_order_relaxed);
    });

    controller_->setMidiConnectionChangedCallback([this](bool /*state*/) {
        devices_dirty_.store(true, std::memory_order_relaxed);
    });

    controller_->setMidiCIPropertiesChangedCallback([this](uint32_t muid, const std::string& propertyId, const std::string& resId) {
        if (propertyId == midicci::commonproperties::StandardPropertyNames::ALL_CTRL_LIST ||
            propertyId == midicci::commonproperties::StandardPropertyNames::PROGRAM_LIST ||
            propertyId == midicci::commonproperties::StandardPropertyNames::CTRL_MAP_LIST) {
            std::lock_guard<std::mutex> lock(property_update_mutex_);
            pending_property_updates_.push_back({muid, propertyId, resId});
        }
    });

    controller_->setStateSaveCallback([this](uint32_t muid, const std::vector<uint8_t>& stateData) {
        on_save_state(muid, stateData);
    });

    devices_dirty_.store(true, std::memory_order_relaxed);
    ci_dirty_.store(true, std::memory_order_relaxed);
}

KeyboardPanel::~KeyboardPanel() {
    if (repository_ && log_bridge_) {
        message_logger_.remove_log_callback(log_bridge_);
    }
    if (note_callback_active_) {
        note_callback_active_->store(false);
    }
    if (controller_) {
        controller_->setIncomingNoteCallback(nullptr);
        controller_->setIncomingControlValueCallback(nullptr);
        controller_->setExternalOutputCallback(nullptr);
        controller_->allNotesOff();
    }
}

void KeyboardPanel::render() {
    apply_pending_updates();

    render_transport_section();
    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Device Information"))
        render_ci_section();
    ImGui::Spacing();
    render_keyboard_section();
    ImGui::Spacing();
    uint32_t current_muid = current_selected_muid();
    render_ci_property_tools(current_muid);
}

void KeyboardPanel::apply_pending_updates() {
    if (devices_dirty_.exchange(false)) {
        refresh_devices();
    }
    if (ci_dirty_.exchange(false)) {
        ctrl_map_cache_.clear();
        ctrl_list_cache_.clear();
        program_list_cache_.clear();
        control_values_.clear();
        identity_values_.clear();
        control_keys_by_device_.clear();
        identity_to_control_keys_.clear();
        control_key_to_identity_.clear();
        refresh_ci_devices();
    }
    process_property_updates();
    process_incoming_note_events();
    process_incoming_control_events();
}

void KeyboardPanel::process_property_updates() {
    std::vector<PendingPropertyUpdate> updates;
    {
        std::lock_guard<std::mutex> lock(property_update_mutex_);
        if (pending_property_updates_.empty()) {
            return;
        }
        updates.swap(pending_property_updates_);
    }

    if (!controller_) {
        return;
    }

    for (const auto& update : updates) {
        if (update.property_id == midicci::commonproperties::StandardPropertyNames::ALL_CTRL_LIST) {
            auto controls = controller_->getAllCtrlList(update.muid);
            if (controls) {
                ctrl_list_cache_[update.muid] = *controls;
                rebuild_control_lookup(update.muid, *controls);
            }
        } else if (update.property_id == midicci::commonproperties::StandardPropertyNames::PROGRAM_LIST) {
            auto programs = controller_->getProgramList(update.muid);
            if (programs) {
                program_list_cache_[update.muid] = *programs;
            }
        } else if (update.property_id == midicci::commonproperties::StandardPropertyNames::CTRL_MAP_LIST) {
            if (update.res_id.empty()) {
                continue;
            }
            auto device_it = ctrl_map_cache_.find(update.muid);
            if (device_it == ctrl_map_cache_.end()) {
                continue;
            }
            auto cache_it = device_it->second.find(update.res_id);
            if (cache_it == device_it->second.end()) {
                continue;
            }
            auto latest = controller_->getCtrlMapList(update.muid, update.res_id);
            auto& cache = cache_it->second;
            cache.pending = false;
            cache.checked_local = true;
            cache.last_request_time = std::chrono::steady_clock::now();
            if (latest) {
                cache.values = *latest;
                cache.loaded = true;
            } else {
                cache.values.clear();
                cache.loaded = false;
            }
        }
    }
}

void KeyboardPanel::enqueue_incoming_note_event(int note, int velocity, bool is_pressed) {
    if (note < 0 || note > 127) {
        return;
    }
    std::lock_guard<std::mutex> lock(incoming_note_mutex_);
    pending_incoming_notes_.push_back({note, velocity, is_pressed});
}

void KeyboardPanel::process_incoming_note_events() {
    std::vector<PendingNoteEvent> events;
    {
        std::lock_guard<std::mutex> lock(incoming_note_mutex_);
        if (pending_incoming_notes_.empty()) {
            return;
        }
        events.swap(pending_incoming_notes_);
    }
    for (const auto& evt : events) {
        midi_keyboard_.set_external_key_state(evt.note, evt.is_pressed);
    }
}

void KeyboardPanel::enqueue_incoming_control_event(const KeyboardController::IncomingControlValue& value) {
    PendingControlValue pending;
    pending.ctrlType = value.ctrlType;
    pending.ctrlIndex = value.ctrlIndex;
    pending.value = value.value;
    pending.note = value.note;
    std::lock_guard<std::mutex> lock(incoming_control_mutex_);
    pending_control_updates_.push_back(std::move(pending));
}

void KeyboardPanel::process_incoming_control_events() {
    std::vector<PendingControlValue> events;
    {
        std::lock_guard<std::mutex> lock(incoming_control_mutex_);
        if (pending_control_updates_.empty()) {
            return;
        }
        events.swap(pending_control_updates_);
    }

    for (const auto& evt : events) {
        std::string identity = build_control_identity(evt.ctrlType, evt.ctrlIndex);
        uint32_t stored_value = evt.value;
        identity_values_[identity] = stored_value;

        auto map_it = identity_to_control_keys_.find(identity);
        if (map_it == identity_to_control_keys_.end()) {
            continue;
        }
        for (const auto& control_key : map_it->second) {
            control_values_[control_key] = stored_value;
        }
    }
}

std::string KeyboardPanel::build_control_identity(const std::string& ctrl_type,
                                                  const std::vector<uint8_t>& index) const {
    std::ostringstream oss;
    oss << ctrl_type << ':';
    oss << std::uppercase << std::hex << std::setfill('0');
    for (uint8_t byte : index) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::string KeyboardPanel::build_control_key(uint32_t muid,
                                             const midicci::commonproperties::MidiCIControl& ctrl) const {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << muid << ':';
    oss << build_control_identity(ctrl.ctrlType, ctrl.ctrlIndex);
    return oss.str();
}

void KeyboardPanel::rebuild_control_lookup(
    uint32_t muid,
    const std::vector<midicci::commonproperties::MidiCIControl>& controls) {
    auto existing = control_keys_by_device_.find(muid);
    if (existing != control_keys_by_device_.end()) {
        for (const auto& key : existing->second) {
            auto identity_it = control_key_to_identity_.find(key);
            if (identity_it != control_key_to_identity_.end()) {
                auto targets_it = identity_to_control_keys_.find(identity_it->second);
                if (targets_it != identity_to_control_keys_.end()) {
                    auto& entries = targets_it->second;
                    entries.erase(std::remove(entries.begin(), entries.end(), key), entries.end());
                    if (entries.empty()) {
                        identity_to_control_keys_.erase(targets_it);
                    }
                }
                control_key_to_identity_.erase(identity_it);
            }
        }
        existing->second.clear();
    }

    auto& device_keys = control_keys_by_device_[muid];
    device_keys.clear();
    device_keys.reserve(controls.size());
    for (const auto& ctrl : controls) {
        std::string identity = build_control_identity(ctrl.ctrlType, ctrl.ctrlIndex);
        std::string control_key = build_control_key(muid, ctrl);
        device_keys.push_back(control_key);
        control_key_to_identity_[control_key] = identity;
        identity_to_control_keys_[identity].push_back(control_key);

        auto identity_value = identity_values_.find(identity);
        if (identity_value != identity_values_.end()) {
            control_values_[control_key] = identity_value->second;
        }
    }
}

void KeyboardPanel::invalidate_invisible_ctrl_map_entries(uint32_t muid, int current_frame) {
    auto device_it = ctrl_map_cache_.find(muid);
    if (device_it == ctrl_map_cache_.end()) {
        return;
    }
    for (auto& entry : device_it->second) {
        if (entry.second.last_visible_frame != current_frame) {
            entry.second.values.clear();
            entry.second.loaded = false;
            entry.second.checked_local = false;
        }
    }
}

void KeyboardPanel::refresh_devices() {
    controller_->refreshDevices();
    auto inputs = controller_->getInputDevices();
    auto outputs = controller_->getOutputDevices();

    std::string ignored_input_name;
    std::string ignored_output_name;
    if (repository_) {
        if (auto midi_manager = repository_->get_midi_device_manager()) {
            ignored_input_name = midi_manager->get_virtual_input_name();
            ignored_output_name = midi_manager->get_virtual_output_name();
        }
    }

    std::lock_guard<std::mutex> lock(state_mutex_);
    input_devices_.clear();
    output_devices_.clear();

    for (const auto& device : inputs) {
        if (!ignored_output_name.empty() && device.second == ignored_output_name) {
            continue;
        }
        input_devices_.push_back(DeviceEntry{device.first, device.second});
    }
    for (const auto& device : outputs) {
        if (!ignored_input_name.empty() && device.second == ignored_input_name) {
            continue;
        }
        output_devices_.push_back(DeviceEntry{device.first, device.second});
    }

    selected_input_index_ = -1;
    if (!current_input_id_.empty()) {
        for (size_t i = 0; i < input_devices_.size(); ++i) {
            if (input_devices_[i].id == current_input_id_) {
                selected_input_index_ = static_cast<int>(i);
                break;
            }
        }
        if (selected_input_index_ == -1) {
            current_input_id_.clear();
        }
    }

    selected_output_index_ = -1;
    if (!current_output_id_.empty()) {
        for (size_t i = 0; i < output_devices_.size(); ++i) {
            if (output_devices_[i].id == current_output_id_) {
                selected_output_index_ = static_cast<int>(i);
                break;
            }
        }
        if (selected_output_index_ == -1) {
            current_output_id_.clear();
        }
    }
}

void KeyboardPanel::refresh_ci_devices() {
    auto devices = controller_->getMidiCIDeviceDetails();
    std::lock_guard<std::mutex> lock(state_mutex_);
    uint32_t previously_selected_muid = 0;
    if (selected_ci_index_ >= 0 && selected_ci_index_ < static_cast<int>(ci_devices_.size())) {
        previously_selected_muid = ci_devices_[selected_ci_index_].muid;
    }

    ci_devices_ = std::move(devices);

    if (suppress_ci_auto_select_) {
        suppress_ci_auto_select_ = false;
        if (selected_ci_index_ >= static_cast<int>(ci_devices_.size())) {
            selected_ci_index_ = -1;
        }
        return;
    }

    if (previously_selected_muid != 0) {
        auto preserved = std::find_if(ci_devices_.begin(), ci_devices_.end(),
                                      [previously_selected_muid](const MidiCIDeviceInfo& info) {
                                          return info.muid == previously_selected_muid;
                                      });
        if (preserved != ci_devices_.end()) {
            selected_ci_index_ = static_cast<int>(std::distance(ci_devices_.begin(), preserved));
            return;
        }
    }

    if (selected_ci_index_ >= static_cast<int>(ci_devices_.size())) {
        selected_ci_index_ = -1;
    }

    if (selected_ci_index_ == -1 && !ci_devices_.empty()) {
        auto ready_device = std::find_if(ci_devices_.begin(), ci_devices_.end(),
                                         [](const MidiCIDeviceInfo& info) {
                                             return info.endpoint_ready;
                                         });
        if (ready_device != ci_devices_.end()) {
            selected_ci_index_ = static_cast<int>(std::distance(ci_devices_.begin(), ready_device));
        } else {
            selected_ci_index_ = 0;
        }
    }
}

void KeyboardPanel::render_transport_section() {
    std::vector<DeviceEntry> inputs;
    std::vector<DeviceEntry> outputs;
    int selected_input = -1;
    int selected_output = -1;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        inputs = input_devices_;
        outputs = output_devices_;
        selected_input = selected_input_index_;
        selected_output = selected_output_index_;
    }

    auto combo = [&](const char* label, int selected_index, const std::vector<DeviceEntry>& entries,
                     auto&& selection_callback) {
        std::string current_label = selected_index >= 0 && selected_index < static_cast<int>(entries.size())
                                        ? entries[selected_index].name
                                        : "Virtual (default)";
        if (ImGui::BeginCombo(label, current_label.c_str())) {
            bool virtual_selected = (selected_index < 0);
            if (ImGui::Selectable("Virtual (default)", virtual_selected)) {
                selection_callback(-1);
            }
            if (virtual_selected) {
                ImGui::SetItemDefaultFocus();
            }

            for (size_t i = 0; i < entries.size(); ++i) {
                bool is_selected = static_cast<int>(i) == selected_index;
                if (ImGui::Selectable(entries[i].name.c_str(), is_selected)) {
                    selection_callback(static_cast<int>(i));
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
    };

    ImGui::TextUnformatted("MIDI Input:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    combo("##input-device", selected_input, inputs, [this](int index) {
        select_input_device(index);
    });
    ImGui::SameLine();
    ImGui::TextUnformatted("Output:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(220.0f);
    combo("##output-device", selected_output, outputs, [this](int index) {
        select_output_device(index);
    });
    ImGui::SameLine();
    if (ImGui::Button("Refresh Devices")) {
        controller_->selectInputDevice("");
        controller_->selectOutputDevice("");

        if (repository_) {
            if (auto midi_manager = repository_->get_midi_device_manager()) {
                midi_manager->set_input_device("");
                midi_manager->set_output_device("");
            }
        }

        {
            std::lock_guard<std::mutex> lock(state_mutex_);
            selected_input_index_ = -1;
            selected_output_index_ = -1;
            current_input_id_.clear();
            current_output_id_.clear();
            selected_ci_index_ = -1;
            suppress_ci_auto_select_ = true;
        }

        devices_dirty_.store(true, std::memory_order_relaxed);
        ci_dirty_.store(true, std::memory_order_relaxed);
    }
}

void KeyboardPanel::render_keyboard_section() {
    midi_keyboard_.render();
}

void KeyboardPanel::render_ci_section() {
    ImGui::Spacing();
    if (ImGui::Button("Send Discovery")) {
        controller_->sendMidiCIDiscovery();
    }

    ImGui::SameLine();
    std::vector<MidiCIDeviceInfo> devices_copy;
    int selected_index = -1;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        devices_copy = ci_devices_;
        selected_index = selected_ci_index_;
    }

    std::vector<std::string> device_labels;
    device_labels.reserve(devices_copy.size());
    for (const auto& device : devices_copy) {
        std::ostringstream label;
        label << device.manufacturer << " " << device.model << " (0x" << std::hex << device.muid << ")";
        device_labels.push_back(label.str());
    }

    ImGui::TextUnformatted("Device:");
    ImGui::SameLine();
    std::string current_label = (selected_index >= 0 && selected_index < static_cast<int>(device_labels.size()))
                                    ? device_labels[selected_index]
                                    : "Select device";
    if (ImGui::BeginCombo("##ci-device", current_label.c_str())) {
        for (size_t i = 0; i < device_labels.size(); ++i) {
            bool is_selected = static_cast<int>(i) == selected_index;
            if (ImGui::Selectable(device_labels[i].c_str(), is_selected)) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                selected_ci_index_ = static_cast<int>(i);
            }
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::Spacing();
    bool midi_ready = controller_->hasValidMidiPair();
    ImGui::Text("Active connection: %s", midi_ready ? "Yes" : "No");

    ImGui::Spacing();
    bool initialized = controller_->isMidiCIInitialized();
    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Local MUID: 0x%08X", controller_->getMidiCIMuid());
    ImGui::Text("Local Device: %s", controller_->getMidiCIDeviceName().c_str());
    ImGui::Text("Initialized: %s", initialized ? "Yes" : "No");
    ImGui::NextColumn();
    render_selected_ci_device();
    ImGui::Columns(1);
}

void KeyboardPanel::render_selected_ci_device() {
    MidiCIDeviceInfo device_snapshot(
        0,
        "",
        "",
        "",
        "",
        0,
        0
    );
    bool has_selection = false;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (selected_ci_index_ >= 0 && selected_ci_index_ < static_cast<int>(ci_devices_.size())) {
            device_snapshot = ci_devices_[selected_ci_index_];
            has_selection = true;
        }
    }

    if (!has_selection) {
        ImGui::TextUnformatted("Select a device to view details.");
        return;
    }

    ImGui::Text("Manufacturer: %s", device_snapshot.manufacturer.c_str());
    ImGui::Text("Model: %s", device_snapshot.model.c_str());
    ImGui::Text("Version: %s", device_snapshot.version.c_str());
    ImGui::Text("Supports Features: 0x%02X", device_snapshot.supported_features);
    ImGui::Text("Max SysEx Size: %u bytes", device_snapshot.max_sysex_size);
}

uint32_t KeyboardPanel::current_selected_muid() const {
    if (selected_ci_index_ < 0 || selected_ci_index_ >= static_cast<int>(ci_devices_.size())) {
        return 0;
    }
    return ci_devices_[selected_ci_index_].muid;
}

void KeyboardPanel::render_ci_property_tools(uint32_t muid) {
    if (controller_ == nullptr) {
        return;
    }

    ImGui::Separator();
    if (muid == 0) {
        ImGui::TextUnformatted("Select a MIDI-CI device to view Control and Program lists.");
        return;
    }
    const ImFont* font = ImGui::GetFont();
    const float ui_scale = font ? std::max(font->Scale, 0.1f) : 1.0f;

    ImGui::BeginChild("state-program-column", ImVec2(0, 100.0f * ui_scale), true);

    if (ImGui::Button("Save State")) {
        if (controller_) {
            controller_->requestSaveState(muid);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load State")) {
        on_load_state(muid);
    }

    const auto program_it = program_list_cache_.find(muid);
    const auto* programs = (program_it != program_list_cache_.end()) ? &program_it->second : nullptr;
    bool has_programs = programs && !programs->empty();
    if (ImGui::Button("Request##prg-list")) {
        controller_->requestProgramList(muid);
    }
    ImGui::SameLine();
    float program_combo_width = ImGui::GetContentRegionAvail().x;
    auto format_program_label = [](const midicci::commonproperties::MidiCIProgram& program) {
        uint8_t msb = program.bankPC.size() > 0 ? program.bankPC[0] : 0;
        uint8_t lsb = program.bankPC.size() > 1 ? program.bankPC[1] : 0;
        uint8_t pc = program.bankPC.size() > 2 ? program.bankPC[2] : 0;
        std::ostringstream oss;
        oss << '[' << static_cast<int>(msb) <<  ':' << static_cast<int>(lsb) << ':' << static_cast<int>(pc) << "] " << program.title;
        return oss.str();
    };
    if (!has_programs) {
        ImGui::BeginDisabled();
    }
    std::string preview_label = "[BankMSB:BankLSB:Program] Select program";
    int current_program_index = -1;
    if (has_programs) {
        auto sel_it = selected_program_index_.find(muid);
        if (sel_it != selected_program_index_.end()) {
            current_program_index = sel_it->second;
        }
        if (current_program_index >= 0 && current_program_index < static_cast<int>(programs->size())) {
            preview_label = format_program_label((*programs)[static_cast<size_t>(current_program_index)]);
        }
    } else {
        preview_label = "No programs";
    }
    ImGui::SetNextItemWidth(program_combo_width);
    if (ImGui::BeginCombo("##program-list", preview_label.c_str())) {
        if (has_programs) {
            for (size_t i = 0; i < programs->size(); ++i) {
                const auto& program = (*programs)[i];
                std::string item_label = format_program_label(program);
                bool selected = (static_cast<int>(i) == current_program_index);
                if (ImGui::Selectable(item_label.c_str(), selected)) {
                    selected_program_index_[muid] = static_cast<int>(i);
                    if (program.bankPC.size() >= 3) {
                        controller_->sendProgramChange(
                            current_channel_value(),
                            program.bankPC[2],
                            program.bankPC[0],
                            program.bankPC[1],
                            current_group_value());
                    }
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }
    if (!has_programs) {
        ImGui::EndDisabled();
    }
    ImGui::EndChild();

    ImGui::Spacing();
    ImGui::BeginChild("control-column", ImVec2(0, 0), true);

    ImGui::TextUnformatted("Control List");
    ImGui::SameLine();
    if (ImGui::Button("Request##ctrl-list")) {
        controller_->requestAllCtrlList(muid);
    }
    if (muid != last_selected_muid_) {
        if (last_selected_muid_ != 0) {
            ctrl_map_cache_.erase(last_selected_muid_);
            ctrl_list_cache_.erase(last_selected_muid_);
            program_list_cache_.erase(last_selected_muid_);
            selected_program_index_.erase(last_selected_muid_);
        }
        last_selected_muid_ = muid;
        // Do not auto-fetch control/program lists; leave them empty until the user requests data.
    }
    ImGui::Spacing();
    render_parameter_context_controls();
    ImGui::Spacing();
    ImGui::AlignTextToFramePadding();
    ImGui::TextUnformatted("Filter Controls:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-FLT_MIN);
    ImGui::InputText("##control-filter", parameter_filter_.data(), parameter_filter_.size());
    ImGui::Spacing();

    const auto ctrl_list_it = ctrl_list_cache_.find(muid);
    const std::vector<midicci::commonproperties::MidiCIControl>* controls = (ctrl_list_it != ctrl_list_cache_.end())
        ? &ctrl_list_it->second
        : nullptr;
    std::string filter_value(parameter_filter_.data());
    std::string filter_lower = filter_value;
    std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const bool filter_active = !filter_lower.empty();
    auto matches_filter = [&](const midicci::commonproperties::MidiCIControl& ctrl) {
        if (!filter_active) {
            return true;
        }
        auto contains_filter = [&](const std::string& value) {
            if (value.empty()) {
                return false;
            }
            std::string lowered = value;
            std::transform(lowered.begin(),
                           lowered.end(),
                           lowered.begin(),
                           [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            return lowered.find(filter_lower) != std::string::npos;
        };
        if (contains_filter(ctrl.title)) {
            return true;
        }
        if (contains_filter(ctrl.description)) {
            return true;
        }
        if (contains_filter(format_parameter_id(ctrl))) {
            return true;
        }
        auto path = format_parameter_path(ctrl);
        if (path != "-" && contains_filter(path)) {
            return true;
        }
        if (!ctrl.ctrlType.empty() && contains_filter(ctrl.ctrlType)) {
            return true;
        }
        if (ctrl.ctrlMapId && contains_filter(*ctrl.ctrlMapId)) {
            return true;
        }
        return false;
    };
    auto is_visible_control = [&](const midicci::commonproperties::MidiCIControl& ctrl) {
        return control_matches_context(ctrl) && matches_filter(ctrl);
    };
    auto has_visible_controls = [&](const std::vector<midicci::commonproperties::MidiCIControl>& list) {
        return std::any_of(list.begin(), list.end(), is_visible_control);
    };
    bool controls_available = controls && !controls->empty();
    bool any_visible_controls = controls_available && has_visible_controls(*controls);

    if (any_visible_controls) {
        const ImGuiTableFlags control_table_flags = ImGuiTableFlags_RowBg |
                                                   ImGuiTableFlags_Borders |
                                                   ImGuiTableFlags_Resizable |
                                                   ImGuiTableFlags_SizingStretchProp;
        if (ImGui::BeginTable("control-table", 4, control_table_flags)) {
            const int current_frame = ImGui::GetFrameCount();
            constexpr float PATH_COLUMN_WIDTH = 110.0f;
            constexpr float ID_COLUMN_WIDTH = 100.0f;
            ImGui::TableSetupColumn("Path",
                                    ImGuiTableColumnFlags_WidthFixed,
                                    PATH_COLUMN_WIDTH);
            ImGui::TableSetupColumn("Param ID",
                                    ImGuiTableColumnFlags_WidthFixed,
                                    ID_COLUMN_WIDTH);
            ImGui::TableSetupColumn("Title", ImGuiTableColumnFlags_WidthStretch, 4.0f);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableHeadersRow();
            int row = 0;
            for (const auto& ctrl : *controls) {
                if (!is_visible_control(ctrl)) {
                    continue;
                }
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                std::string param_path = format_parameter_path(ctrl);
                ImGui::TextUnformatted(param_path.c_str());
                if (param_path != "-" && ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("%s", param_path.c_str());
                }
                ImGui::TableSetColumnIndex(1);
                std::string parameter_id = format_parameter_id(ctrl);
                ImGui::TextUnformatted(parameter_id.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(ctrl.title.c_str());
                ImGui::TableSetColumnIndex(3);
                std::string key = build_control_key(muid, ctrl);
                uint32_t min_raw = ctrl.minMax.size() > 0 ? ctrl.minMax[0] : 0u;
                uint32_t max_raw = ctrl.minMax.size() > 1 ? ctrl.minMax[1] : std::numeric_limits<uint32_t>::max();
                if (max_raw < min_raw) std::swap(max_raw, min_raw);
                uint64_t span = static_cast<uint64_t>(max_raw) - static_cast<uint64_t>(min_raw);
                uint64_t num_bits = static_cast<uint64_t>(std::clamp(ctrl.numSigBits, 1, 32));
                uint64_t max_bits_range = (num_bits >= 32) ? std::numeric_limits<uint32_t>::max()
                                                           : ((1ull << num_bits) - 1ull);
                if (span > max_bits_range) {
                    max_raw = static_cast<uint32_t>(min_raw + max_bits_range);
                    span = max_bits_range;
                }

                auto found = control_values_.find(key);
                if (found == control_values_.end()) {
                    uint32_t def = static_cast<uint32_t>(std::clamp<uint64_t>(ctrl.defaultValue, min_raw, max_raw));
                    found = control_values_.emplace(key, def).first;
                }
                uint32_t& value = found->second;
                float width = ImGui::GetContentRegionAvail().x;
                ImGui::PushID(row);
                const bool has_combo = ctrl.ctrlMapId.has_value();
                ControlMapCache* ctrl_map_cache_entry = nullptr;
                std::string ctrl_map_id_str;
                float combo_button_width = 0.0f;
                float combo_spacing = 0.0f;
                if (has_combo) {
                    combo_button_width = ImGui::GetFrameHeight();
                    combo_spacing = ImGui::GetStyle().ItemInnerSpacing.x;
                    width = std::max(20.0f, width - (combo_button_width + combo_spacing));
                    ctrl_map_id_str = *ctrl.ctrlMapId;
                    ctrl_map_cache_entry = &ctrl_map_cache_[muid][ctrl_map_id_str];
                    ctrl_map_cache_entry->last_visible_frame = current_frame;
                }
                ImGui::SetNextItemWidth(width);
                float slider_ratio = 0.0f;
                if (span > 0) {
                    slider_ratio = static_cast<float>((static_cast<double>(value) - static_cast<double>(min_raw)) / static_cast<double>(span));
                }
                bool value_changed = ImGui::SliderFloat("##ctrl", &slider_ratio, 0.0f, 1.0f, "%.3f");
                ImVec2 slider_min = ImGui::GetItemRectMin();
                ImVec2 slider_max = ImGui::GetItemRectMax();
                if (value_changed) {
                    uint64_t raw = static_cast<uint64_t>(min_raw) + static_cast<uint64_t>(slider_ratio * static_cast<float>(span));
                    raw = std::min<uint64_t>(raw, max_raw);
                    value = static_cast<uint32_t>(raw);
                    send_control_value(ctrl, static_cast<uint32_t>(raw));
                }
                if (has_combo) {
                    ImGui::SameLine(0.0f, combo_spacing);
                    std::string combo_button = "##link-btn-" + std::to_string(row);
                    std::string combo_popup = "##link-popup-" + std::to_string(row);
                    if (ImGui::ArrowButton(combo_button.c_str(), ImGuiDir_Down)) {
                        if (ImGui::IsPopupOpen(combo_popup.c_str(), ImGuiPopupFlags_None)) {
                            ImGui::CloseCurrentPopup();
                        } else {
                            ImGui::OpenPopup(combo_popup.c_str());
                        }
                    }
                    ImGui::SetNextWindowPos(slider_min);
                    ImGui::SetNextWindowSize(ImVec2(slider_max.x - slider_min.x, 0.0f));
                    if (ImGui::BeginPopup(combo_popup.c_str())) {
                        const std::vector<midicci::commonproperties::MidiCIControlMap>* map_values = nullptr;
                        bool map_loading = false;
                        if (ctrl_map_cache_entry) {
                            auto& cache = *ctrl_map_cache_entry;
                            auto now = std::chrono::steady_clock::now();
                            if (cache.pending && cache.last_request_time.time_since_epoch().count() > 0 &&
                                (now - cache.last_request_time) > CTRL_MAP_REQUEST_TIMEOUT) {
                                cache.pending = false;
                                cache.checked_local = false;
                            }
                            if (!cache.loaded && !cache.checked_local && controller_) {
                                auto latest = controller_->getCtrlMapList(muid, ctrl_map_id_str);
                                cache.checked_local = true;
                                if (latest) {
                                    cache.values = *latest;
                                    cache.loaded = true;
                                    cache.pending = false;
                                } else {
                                    cache.values.clear();
                                    cache.loaded = false;
                                }
                            }
                            if (!cache.loaded && !cache.pending && controller_) {
                                controller_->requestCtrlMapList(muid, ctrl_map_id_str);
                                cache.pending = true;
                                cache.last_request_time = now;
                            }
                            if (cache.loaded && !cache.values.empty()) {
                                map_values = &cache.values;
                            }
                            map_loading = cache.pending && !cache.loaded;
                        }
                        if (map_values && !map_values->empty()) {
                            uint32_t current_value = value;
                            for (const auto& mapEntry : *map_values) {
                                bool selected = current_value == mapEntry.value;
                                if (ImGui::Selectable(mapEntry.title.c_str(), selected)) {
                                    value = mapEntry.value;
                                    send_control_value(ctrl, mapEntry.value);
                                    ImGui::CloseCurrentPopup();
                                }
                                if (selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                        } else if (map_loading) {
                            ImGui::TextUnformatted("Loading control map...");
                        } else {
                            ImGui::TextUnformatted("No mappings available.");
                        }
                        ImGui::EndPopup();
                    }
                }
                ImGui::PopID();
                (void)ctrl.channel;
                ++row;
            }
            ImGui::EndTable();
            invalidate_invisible_ctrl_map_entries(muid, current_frame);
        }
    } else {
        invalidate_invisible_ctrl_map_entries(muid, -1);
        if (!controls_available) {
            ImGui::TextUnformatted("Control data not received yet.");
        } else if (filter_active) {
            ImGui::TextUnformatted("No controls match filter.");
        } else {
            ImGui::TextUnformatted("No controls for this context.");
        }
    }
    ImGui::EndChild();
}

void KeyboardPanel::render_parameter_context_controls() {
    const ImFont* font = ImGui::GetFont();
    const float ui_scale = font ? std::max(font->Scale, 0.1f) : 1.0f;
    const float context_column_width = 360.0f * ui_scale;
    const float context_combo_width = 140.0f * ui_scale;
    const float context_value_width = 160.0f * ui_scale;

    const ImGuiTableFlags table_flags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoSavedSettings;
    if (ImGui::BeginTable("parameter-context-layout", 2, table_flags)) {
        ImGui::TableSetupColumn("context-controls", ImGuiTableColumnFlags_WidthFixed, context_column_width);
        ImGui::TableSetupColumn("context-keyboard", ImGuiTableColumnFlags_WidthStretch, 0.0f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        auto format_value_label = [&](int idx) {
            idx = std::clamp(idx, 0, 127);
            if (parameter_context_ == ParameterContext::Key) {
                std::ostringstream oss;
                oss << idx << " (" << note_label(idx) << ")";
                return oss.str();
            }
            return std::to_string(idx);
        };

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted("Context:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(context_combo_width);
        int context_index = std::clamp(static_cast<int>(parameter_context_), 0, PARAM_CONTEXT_COUNT - 1);
        const char* current_label = PARAM_CONTEXT_LABELS[context_index];
        if (ImGui::BeginCombo("##parameter-context", current_label)) {
            for (int i = 0; i < PARAM_CONTEXT_COUNT; ++i) {
                bool selected = (context_index == i);
                if (ImGui::Selectable(PARAM_CONTEXT_LABELS[i], selected)) {
                    parameter_context_ = static_cast<ParameterContext>(i);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::SameLine();
        ImGui::TextUnformatted("Value/Key:");
        ImGui::SameLine();

        bool disable_combo = (parameter_context_ == ParameterContext::Global);
        if (disable_combo) {
            ImGui::BeginDisabled();
        }

        int* target_value = nullptr;
        int max_items = 0;
        switch (parameter_context_) {
        case ParameterContext::Group:
            target_value = &parameter_group_value_;
            max_items = 16;
            break;
        case ParameterContext::Channel:
            target_value = &parameter_channel_value_;
            max_items = 16;
            break;
        case ParameterContext::Key:
            target_value = &parameter_key_value_;
            max_items = 128;
            break;
        case ParameterContext::Global:
            break;
        }

        int current_value = 0;
        if (target_value != nullptr && max_items > 0) {
            current_value = std::clamp(*target_value, 0, max_items - 1);
            *target_value = current_value;
        }
        std::string preview_label = (target_value != nullptr && max_items > 0)
                                        ? format_value_label(current_value)
                                        : std::string("-");

        ImGui::SetNextItemWidth(context_value_width);
        if (ImGui::BeginCombo("##parameter-context-value", preview_label.c_str())) {
            for (int idx = 0; idx < max_items; ++idx) {
                bool is_selected = (target_value != nullptr && idx == current_value);
                std::string option = format_value_label(idx);
                if (ImGui::Selectable(option.c_str(), is_selected) && target_value != nullptr) {
                    *target_value = idx;
                    if (parameter_context_ == ParameterContext::Key) {
                        set_parameter_key_value(*target_value);
                    }
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (disable_combo) {
            ImGui::EndDisabled();
        }

        ImGui::TableSetColumnIndex(1);
        ImGui::PushID("parameter-keyboard");
        parameter_keyboard_.render();
        ImGui::PopID();

        ImGui::EndTable();
    }
}

int KeyboardPanel::current_group_value() const {
    return std::clamp(parameter_group_value_, 0, 15);
}

int KeyboardPanel::current_channel_value() const {
    return std::clamp(parameter_channel_value_, 0, 15);
}

int KeyboardPanel::current_key_value() const {
    return std::clamp(parameter_key_value_, 0, 127);
}

int KeyboardPanel::resolve_channel(const midicci::commonproperties::MidiCIControl& ctrl) const {
    if (parameter_context_ == ParameterContext::Channel || parameter_context_ == ParameterContext::Key) {
        return current_channel_value();
    }
    if (ctrl.channel.has_value()) {
        return std::clamp(static_cast<int>(*ctrl.channel), 0, 15);
    }
    return 0;
}

void KeyboardPanel::send_control_value(const midicci::commonproperties::MidiCIControl& ctrl, uint32_t value) {
    if (!controller_) {
        return;
    }
    const int group = current_group_value();
    const int channel = resolve_channel(ctrl);
    const auto& type = ctrl.ctrlType;

    if (type == midicci::commonproperties::MidiCIControlType::RPN) {
        if (ctrl.ctrlIndex.size() >= 2) {
            controller_->sendRPN(channel, ctrl.ctrlIndex[0], ctrl.ctrlIndex[1], value, group);
        }
    } else if (type == midicci::commonproperties::MidiCIControlType::NRPN) {
        if (ctrl.ctrlIndex.size() >= 2) {
            controller_->sendNRPN(channel, ctrl.ctrlIndex[0], ctrl.ctrlIndex[1], value, group);
        }
    } else if (type == midicci::commonproperties::MidiCIControlType::PNAC) {
        if (!ctrl.ctrlIndex.empty()) {
            controller_->sendPerNoteControlChange(channel, current_key_value(), ctrl.ctrlIndex[0], value, group);
        }
    } else {
        if (!ctrl.ctrlIndex.empty()) {
            controller_->sendControlChange(channel, ctrl.ctrlIndex[0], value, group);
        }
    }
}

void KeyboardPanel::set_parameter_key_value(int note) {
    parameter_key_value_ = std::clamp(note, 0, 127);
    parameter_keyboard_.set_highlighted_key(parameter_key_value_);
}

bool KeyboardPanel::control_matches_context(const midicci::commonproperties::MidiCIControl& ctrl) const {
    const auto& type = ctrl.ctrlType;
    switch (parameter_context_) {
    case ParameterContext::Global:
        return type == midicci::commonproperties::MidiCIControlType::NRPN;
    case ParameterContext::Key:
        return type == midicci::commonproperties::MidiCIControlType::PNAC;
    case ParameterContext::Group:
    case ParameterContext::Channel:
        return true;
    }
    return true;
}

void KeyboardPanel::select_input_device(int index) {
    std::string target_id;
    std::string device_name;
    if (index >= 0) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (index >= static_cast<int>(input_devices_.size())) {
            return;
        }
        target_id = input_devices_[index].id;
        device_name = input_devices_[index].name;
    }

    if (controller_->selectInputDevice(target_id)) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (index < 0) {
            selected_input_index_ = -1;
            current_input_id_.clear();
        } else {
            selected_input_index_ = index;
            current_input_id_ = target_id;
        }
    }

    if (repository_) {
        if (auto midi_manager = repository_->get_midi_device_manager()) {
            midi_manager->set_input_device(device_name);
        }
    }

    check_and_auto_connect();
}

void KeyboardPanel::select_output_device(int index) {
    std::string target_id;
    std::string device_name;
    if (index >= 0) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (index >= static_cast<int>(output_devices_.size())) {
            return;
        }
        target_id = output_devices_[index].id;
        device_name = output_devices_[index].name;
    }

    if (controller_->selectOutputDevice(target_id)) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (index < 0) {
            selected_output_index_ = -1;
            current_output_id_.clear();
        } else {
            selected_output_index_ = index;
            current_output_id_ = target_id;
        }
    }

    if (repository_) {
        if (auto midi_manager = repository_->get_midi_device_manager()) {
            midi_manager->set_output_device(device_name);
        }
    }

    check_and_auto_connect();
}

void KeyboardPanel::on_save_state(uint32_t muid, const std::vector<uint8_t>& stateData) {
    std::string device_model;
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        for (const auto& device : ci_devices_) {
            if (device.muid == muid) {
                device_model = device.model;
                break;
            }
        }
    }

    std::string sanitized = device_model.empty() ? "device" : device_model;
    const std::string illegalChars = R"(<>:"/\|?*)";
    for (char ch : illegalChars) {
        std::replace(sanitized.begin(), sanitized.end(), ch, '-');
    }
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(),
        [](unsigned char c) { return c < 0x20; }), sanitized.end());

    std::string default_filename = "State - " + sanitized + ".state";

    auto save = pfd::save_file(
        "Save Device State",
        default_filename,
        {"State Files", "*.state", "All Files", "*"}
    );

    std::string filename = save.result();
    if (filename.empty()) {
        return;
    }

    if (filename.find(".state") == std::string::npos) {
        filename += ".state";
    }

    std::ofstream outfile(filename, std::ios::binary);
    if (!outfile) {
        pfd::message("Save State", "Failed to open file for writing:\n" + filename, pfd::choice::ok, pfd::icon::error);
        return;
    }

    outfile.write(reinterpret_cast<const char*>(stateData.data()), static_cast<std::streamsize>(stateData.size()));
    outfile.close();

    if (!outfile.good()) {
        pfd::message("Save State", "Failed to write data to file:\n" + filename, pfd::choice::ok, pfd::icon::error);
        return;
    }

    if (repository_) {
        repository_->log("Saved device state to: " + filename, tooling::MessageDirection::Out);
    }
}

void KeyboardPanel::on_load_state(uint32_t muid) {
    auto selection = pfd::open_file(
        "Load Device State",
        "",
        {"State Files", "*.state", "All Files", "*"}
    );

    std::vector<std::string> files = selection.result();
    if (files.empty()) {
        return;
    }

    std::string filename = files[0];
    std::ifstream infile(filename, std::ios::binary | std::ios::ate);
    if (!infile) {
        pfd::message("Load State", "Failed to open file for reading: " + filename, pfd::choice::ok, pfd::icon::error);
        return;
    }

    std::streamsize size = infile.tellg();
    infile.seekg(0, std::ios::beg);

    if (size <= 0) {
        pfd::message("Load State", "File is empty or cannot determine size", pfd::choice::ok, pfd::icon::error);
        return;
    }

    std::vector<uint8_t> stateData(static_cast<size_t>(size));
    if (!infile.read(reinterpret_cast<char*>(stateData.data()), size)) {
        pfd::message("Load State", "Failed to read file contents", pfd::choice::ok, pfd::icon::error);
        return;
    }
    infile.close();

    if (controller_) {
        controller_->sendState(muid, midicci::commonproperties::MidiCIStatePredefinedNames::FULL_STATE, stateData);
        if (repository_) {
            repository_->log("Loaded device state from: " + filename, tooling::MessageDirection::Out);
        }
    }
}

std::string KeyboardPanel::normalize_device_name(const std::string& device_name) {
    if (device_name.length() >= 3 && device_name.substr(device_name.length() - 3) == " In")
        return device_name.substr(0, device_name.length() - 3);
    if (device_name.length() >= 4 && device_name.substr(device_name.length() - 4) == " Out")
        return device_name.substr(0, device_name.length() - 4);
    return device_name;
}

void KeyboardPanel::check_and_auto_connect() {
    if (!controller_) {
        return;
    }

    std::string input_device_name;
    std::string output_device_name;

    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (selected_input_index_ >= 0 && selected_input_index_ < static_cast<int>(input_devices_.size())) {
            input_device_name = input_devices_[selected_input_index_].name;
        }

        if (selected_output_index_ >= 0 && selected_output_index_ < static_cast<int>(output_devices_.size())) {
            output_device_name = output_devices_[selected_output_index_].name;
        }
    }

    if (input_device_name.empty() || output_device_name.empty()) {
        return;
    }

    std::string normalized_input = normalize_device_name(input_device_name);
    std::string normalized_output = normalize_device_name(output_device_name);

    if (normalized_input == normalized_output) {
        if (repository_) {
            repository_->log("Auto-connecting: matched devices '" + input_device_name +
                           "' and '" + output_device_name + "'", tooling::MessageDirection::Out);
        }

        if (controller_->isMidiCIInitialized()) {
            controller_->sendMidiCIDiscovery();
            if (repository_) {
                repository_->log("Automatically sending discovery inquiry", tooling::MessageDirection::Out);
            }
        }
    }
}

} // namespace midicci::app
