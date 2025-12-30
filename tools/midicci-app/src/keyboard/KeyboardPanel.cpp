#include "KeyboardPanel.hpp"

#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <iomanip>
#include <limits>
#include <sstream>
#include <midicci/details/commonproperties/StandardProperties.hpp>

namespace midicci::app {

namespace {
int convert_velocity_to_16bit(int value7) {
    int clamped = std::clamp(value7, 1, 127);
    return clamped << 9;
}

constexpr auto CTRL_MAP_REQUEST_TIMEOUT = std::chrono::seconds(3);
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

    devices_dirty_.store(true, std::memory_order_relaxed);
    ci_dirty_.store(true, std::memory_order_relaxed);
}

KeyboardPanel::~KeyboardPanel() {
    if (repository_ && log_bridge_) {
        message_logger_.remove_log_callback(log_bridge_);
    }
    if (controller_) {
        controller_->allNotesOff();
    }
}

void KeyboardPanel::render() {
    apply_pending_updates();

    render_transport_section();
    ImGui::Spacing();
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
        refresh_ci_devices();
    }
    process_property_updates();
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

    std::lock_guard<std::mutex> lock(state_mutex_);
    input_devices_.clear();
    output_devices_.clear();

    for (const auto& device : inputs) {
        input_devices_.push_back(DeviceEntry{device.first, device.second});
    }
    for (const auto& device : outputs) {
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
    ci_devices_ = devices;
    if (selected_ci_index_ >= static_cast<int>(ci_devices_.size())) {
        selected_ci_index_ = -1;
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
    ImGui::Spacing();
    if (ImGui::Button("Send Discovery")) {
        controller_->sendMidiCIDiscovery();
    }
    ImGui::SameLine();
    if (ImGui::Button("Refresh Devices")) {
        devices_dirty_.store(true, std::memory_order_relaxed);
    }
    ImGui::SameLine();
    bool midi_ready = controller_->hasValidMidiPair();
    ImGui::Text("Active connection: %s", midi_ready ? "Yes" : "No");
}

void KeyboardPanel::render_keyboard_section() {
    midi_keyboard_.render();
}

void KeyboardPanel::render_ci_section() {
    bool initialized = controller_->isMidiCIInitialized();
    ImGui::Columns(2, nullptr, false);
    ImGui::Text("Local MUID: 0x%08X", controller_->getMidiCIMuid());
    ImGui::Text("Local Device: %s", controller_->getMidiCIDeviceName().c_str());
    ImGui::Text("Initialized: %s", initialized ? "Yes" : "No");
    ImGui::NextColumn();
    render_selected_ci_device();
    ImGui::Columns(1);

    ImGui::TextUnformatted("MIDI-CI Devices");

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

    float full_width = ImGui::GetContentRegionAvail().x;
    float half_width = full_width * 0.5f;

    ImGui::BeginChild("control-column", ImVec2(half_width, 280.0f), true);
    ImGui::TextUnformatted("Control List");
    ImGui::SameLine();
    if (ImGui::Button("Request##ctrl-list")) {
        controller_->requestAllCtrlList(muid);
    }
    ImGui::BeginChild("control-scroll", ImVec2(-FLT_MIN, 230.0f), true);
    if (muid != last_selected_muid_) {
        if (last_selected_muid_ != 0) {
            ctrl_map_cache_.erase(last_selected_muid_);
            ctrl_list_cache_.erase(last_selected_muid_);
            program_list_cache_.erase(last_selected_muid_);
        }
        last_selected_muid_ = muid;
        if (controller_ && muid != 0) {
            if (auto initial_controls = controller_->getAllCtrlList(muid)) {
                ctrl_list_cache_[muid] = *initial_controls;
            }
            if (auto initial_programs = controller_->getProgramList(muid)) {
                program_list_cache_[muid] = *initial_programs;
            }
        }
    }

    const auto ctrl_list_it = ctrl_list_cache_.find(muid);
    const std::vector<midicci::commonproperties::MidiCIControl>* controls = (ctrl_list_it != ctrl_list_cache_.end())
        ? &ctrl_list_it->second
        : nullptr;
    if (controls && !controls->empty()) {
        if (ImGui::BeginTable("control-table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            const int current_frame = ImGui::GetFrameCount();
            ImGui::TableSetupColumn("Index");
            ImGui::TableSetupColumn("Title");
            ImGui::TableSetupColumn("Type");
            ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            int row = 0;
            for (const auto& ctrl : *controls) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("%d", row);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(ctrl.title.c_str());
                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(ctrl.ctrlType.c_str());
                ImGui::TableSetColumnIndex(3);
                std::string key = ctrl.title + std::to_string(row);
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
                    found = control_values_.emplace(key, static_cast<int>(def)).first;
                }
                int& value = found->second;
                float width = ImGui::GetContentRegionAvail().x;
                ImGui::PushID(row);
                const bool has_combo = ctrl.ctrlMapId.has_value();
                const std::vector<midicci::commonproperties::MidiCIControlMap>* map_values = nullptr;
                bool map_loading = false;
                float combo_button_width = 0.0f;
                float combo_spacing = 0.0f;
                if (has_combo) {
                    combo_button_width = ImGui::GetFrameHeight();
                    combo_spacing = ImGui::GetStyle().ItemInnerSpacing.x;
                    width = std::max(20.0f, width - (combo_button_width + combo_spacing));
                }
                ImGui::SetNextItemWidth(width);
                float slider_ratio = 0.0f;
                if (span > 0) {
                    slider_ratio = static_cast<float>((static_cast<int64_t>(value) - static_cast<int64_t>(min_raw)) / static_cast<double>(span));
                }
                bool value_changed = ImGui::SliderFloat("##ctrl", &slider_ratio, 0.0f, 1.0f, "%.3f");
                ImVec2 slider_min = ImGui::GetItemRectMin();
                ImVec2 slider_max = ImGui::GetItemRectMax();
                bool slider_visible = ImGui::IsItemVisible();
                if (value_changed) {
                    uint64_t raw = static_cast<uint64_t>(min_raw) + static_cast<uint64_t>(slider_ratio * static_cast<float>(span));
                    raw = std::min<uint64_t>(raw, max_raw);
                    value = static_cast<int>(raw);
                    if (!ctrl.ctrlIndex.empty()) {
                        controller_->sendControlChange(0, ctrl.ctrlIndex[0], static_cast<uint32_t>(raw));
                    }
                }
                if (has_combo && slider_visible) {
                    auto& cache = ctrl_map_cache_[muid][*ctrl.ctrlMapId];
                    cache.last_visible_frame = current_frame;
                    if (cache.pending && cache.last_request_time.time_since_epoch().count() > 0) {
                        auto now = std::chrono::steady_clock::now();
                        if ((now - cache.last_request_time) > CTRL_MAP_REQUEST_TIMEOUT) {
                            cache.pending = false;
                            cache.checked_local = false;
                        }
                    }
                    if (!cache.loaded && !cache.checked_local) {
                        auto latest = controller_->getCtrlMapList(muid, *ctrl.ctrlMapId);
                        cache.checked_local = true;
                        if (latest) {
                            cache.values = *latest;
                            cache.loaded = true;
                        } else {
                            cache.values.clear();
                            cache.loaded = false;
                        }
                    }
                    if (!cache.loaded && !cache.pending) {
                        controller_->requestCtrlMapList(muid, *ctrl.ctrlMapId);
                        cache.pending = true;
                        cache.last_request_time = std::chrono::steady_clock::now();
                    }
                    if (cache.loaded && !cache.values.empty()) {
                        map_values = &cache.values;
                    }
                    map_loading = cache.pending && !cache.loaded;
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
                        if (map_values && !map_values->empty()) {
                            uint32_t current_value = static_cast<uint32_t>(value);
                            for (const auto& mapEntry : *map_values) {
                                bool selected = current_value == mapEntry.value;
                                if (ImGui::Selectable(mapEntry.title.c_str(), selected)) {
                                    value = static_cast<int>(mapEntry.value);
                                    if (!ctrl.ctrlIndex.empty()) {
                                        controller_->sendControlChange(0, ctrl.ctrlIndex[0], mapEntry.value);
                                    }
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
        ImGui::TextUnformatted("Control data not received yet.");
    }
    ImGui::EndChild();
    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("program-column", ImVec2(half_width, 280.0f), true);
    ImGui::TextUnformatted("Program List");
    ImGui::SameLine();
    if (ImGui::Button("Request##prg-list")) {
        controller_->requestProgramList(muid);
    }
    ImGui::BeginChild("program-scroll", ImVec2(-FLT_MIN, 230.0f), true);
    const auto program_it = program_list_cache_.find(muid);
    const std::vector<midicci::commonproperties::MidiCIProgram>* programs = (program_it != program_list_cache_.end())
        ? &program_it->second
        : nullptr;
    if (programs && !programs->empty()) {
        if (ImGui::BeginTable("program-table", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable)) {
            ImGui::TableSetupColumn("Index");
            ImGui::TableSetupColumn("Title");
            ImGui::TableSetupColumn("Bank/Program");
            ImGui::TableHeadersRow();
            int row = 0;
            for (const auto& program : *programs) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                bool selected = ImGui::Selectable(("##prog" + std::to_string(row)).c_str(), false, ImGuiSelectableFlags_SpanAllColumns);
                ImGui::SameLine();
                ImGui::Text("%d", row);
                ImGui::TableSetColumnIndex(1);
                ImGui::TextUnformatted(program.title.c_str());
                ImGui::TableSetColumnIndex(2);
                if (program.bankPC.size() >= 3) {
                    ImGui::Text("MSB %u / LSB %u / PC %u",
                                program.bankPC[0],
                                program.bankPC[1],
                                program.bankPC[2]);
                } else {
                    ImGui::TextUnformatted("-");
                }
                if (selected && program.bankPC.size() >= 3) {
                    controller_->sendProgramChange(0, program.bankPC[2], program.bankPC[0], program.bankPC[1]);
                }
                ++row;
            }
            ImGui::EndTable();
        }
    } else {
        ImGui::TextUnformatted("Program data not received yet.");
    }
    ImGui::EndChild();
    ImGui::EndChild();
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
}

} // namespace midicci::app
