#include "InspectorPanel.hpp"

#include <algorithm>
#include <cfloat>
#include <sstream>

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

#include <midicci/details/ClientConnection.hpp>
#include <midicci/details/MidiCIDevice.hpp>
#include <midicci/details/PropertyValue.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyMetadata.hpp>
#include <midicci/details/commonproperties/PropertyMetadata.hpp>

namespace midicci::app {

namespace {
std::string resolve_property_id(const midicci::commonproperties::PropertyMetadata* metadata) {
    using midicci::commonproperties::CommonRulesPropertyMetadata;
    if (!metadata) {
        return {};
    }
    if (auto* rules = dynamic_cast<const CommonRulesPropertyMetadata*>(metadata)) {
        return rules->resource;
    }
    return metadata->getPropertyId();
}

std::string resolve_property_description(const midicci::commonproperties::PropertyMetadata* metadata) {
    if (!metadata) {
        return {};
    }
    auto description = metadata->getExtra("description");
    if (description.empty()) {
        description = metadata->getExtra("title");
    }
    return description;
}
} // namespace

InspectorPanel::InspectorPanel(tooling::CIToolRepository* repository)
    : repository_(repository) {
    device_model();
}

void InspectorPanel::render() {
    if (!repository_) {
        ImGui::TextUnformatted("CIToolRepository unavailable.");
        return;
    }

    auto model = device_model();
    if (!model) {
        ImGui::TextUnformatted("CI device model unavailable.");
        return;
    }

    std::vector<ConnectionEntry> connections;
    auto list = model->get_connections().to_vector();
    for (const auto& conn : list) {
        if (!conn || !conn->get_connection()) {
            continue;
        }
        ConnectionEntry entry;
        entry.connection = conn;
        entry.muid = conn->get_connection()->get_target_muid();
        std::ostringstream label;
        label << "0x" << std::hex << entry.muid;
        auto info = conn->get_connection()->get_device_info();
        if (info) {
            label << " - " << info->manufacturer_id << ":" << info->family_id;
        }
        entry.label = label.str();
        connections.push_back(std::move(entry));
    }

    if (connections_dirty_.exchange(false)) {
        ensure_connection_selection(connections.size());
        selected_property_id_.clear();
        property_value_buffer_.clear();
    } else {
        ensure_connection_selection(connections.size());
    }

    render_discovery_section(connections);

    if (connections.empty()) {
        ImGui::TextUnformatted("No MIDI-CI devices discovered yet.");
        return;
    }

    ImGui::TextUnformatted("MIDI-CI Device:");
    ImGui::SameLine();
    std::string current_connection_label =
        (selected_connection_index_ >= 0 &&
         selected_connection_index_ < static_cast<int>(connections.size()))
            ? connections[static_cast<size_t>(selected_connection_index_)].label
            : "Select device";
    if (ImGui::BeginCombo("##inspector-ci-device", current_connection_label.c_str())) {
        for (size_t i = 0; i < connections.size(); ++i) {
            bool selected = static_cast<int>(i) == selected_connection_index_;
            if (ImGui::Selectable(connections[i].label.c_str(), selected)) {
                selected_connection_index_ = static_cast<int>(i);
                selected_property_id_.clear();
                property_value_buffer_.clear();
                refresh_property_value(connections[i].connection);
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }
    ImGui::Spacing();

    if (selected_connection_index_ < 0 || selected_connection_index_ >= static_cast<int>(connections.size())) {
        return;
    }

    const auto& current = connections[selected_connection_index_];
    render_device_details(current);
    ImGui::Spacing();
    render_profiles(current);
    ImGui::Spacing();
    render_properties(current);
    ImGui::Spacing();
    render_process_inquiry(current);
}

void InspectorPanel::render_discovery_section(const std::vector<ConnectionEntry>& connections) {
    if (ImGui::Button("Send Discovery")) {
        if (auto model = device_model()) {
            model->send_discovery();
        } else if (repository_) {
            repository_->log("Inspector: CI device model unavailable for discovery",
                             tooling::MessageDirection::Out);
        }
    }
    ImGui::SameLine();
    ImGui::Text("Discovered: %zu", connections.size());
}

void InspectorPanel::render_device_details(const ConnectionEntry& entry) {
    ImGui::Text("Selected MUID: 0x%08X", entry.muid);
    auto conn = entry.connection ? entry.connection->get_connection() : nullptr;
    if (!conn) {
        ImGui::TextUnformatted("Connection unavailable.");
        return;
    }

    const auto* info = conn->get_device_info();
    if (info) {
        ImGui::Text("Manufacturer ID: 0x%06X", info->manufacturer_id);
        ImGui::Text("Family ID: 0x%04X", info->family_id);
        ImGui::Text("Model ID: 0x%04X", info->model_id);
        ImGui::Text("Version ID: 0x%08X", info->version_id);
    } else {
        ImGui::TextUnformatted("Device info pending.");
    }
}

void InspectorPanel::render_profiles(const ConnectionEntry& entry) {
    ImGui::TextUnformatted("Profiles");
    if (!entry.connection) {
        ImGui::TextUnformatted("No active connection.");
        return;
    }
    auto profiles = entry.connection->get_profiles().to_vector();
    if (profiles.empty()) {
        ImGui::TextUnformatted("No profile information yet.");
        return;
    }

    if (ImGui::BeginTable("profiles-table", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Group");
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Profile");
        ImGui::TableSetupColumn("Enabled");
        ImGui::TableSetupColumn("Channels");
        ImGui::TableHeadersRow();

        for (const auto& profile : profiles) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%d", profile->group().get());
            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%d", profile->address().get());
            ImGui::TableSetColumnIndex(2);
            ImGui::TextUnformatted(profile->get_profile().to_string().c_str());
            ImGui::TableSetColumnIndex(3);
            bool enabled = profile->enabled().get();
            if (ImGui::Checkbox(("##prof-en-" + profile->get_profile().to_string()).c_str(), &enabled)) {
                entry.connection->set_profile(
                    profile->group().get(),
                    profile->address().get(),
                    profile->get_profile(),
                    enabled,
                    profile->num_channels_requested().get()
                );
            }
            ImGui::TableSetColumnIndex(4);
            ImGui::Text("%u", profile->num_channels_requested().get());
        }
        ImGui::EndTable();
    }
}

void InspectorPanel::render_properties(const ConnectionEntry& entry) {
    ImGui::TextUnformatted("Properties");
    if (!entry.connection) {
        ImGui::TextUnformatted("No active connection.");
        return;
    }
    if (properties_dirty_.exchange(false)) {
        selected_property_id_.clear();
        property_value_buffer_.clear();
    }
    auto metadata_list = entry.connection->get_metadata_list();
    float total_width = ImGui::GetContentRegionAvail().x;
    float list_width = std::max(0.0f, total_width * 0.25f);

    if (ImGui::BeginChild("property-list-pane", ImVec2(list_width, 0), true)) {
        if (metadata_list.empty()) {
            ImGui::TextUnformatted("No properties advertised yet.");
        } else if (ImGui::BeginListBox("Property Catalog", ImVec2(-FLT_MIN, 160.0f))) {
            for (size_t i = 0; i < metadata_list.size(); ++i) {
                const auto* meta = metadata_list[i].get();
                if (!meta) {
                    continue;
                }
                std::string property_id = resolve_property_id(meta);
                bool selected = (selected_property_id_ == property_id);
                std::string label = property_id;
                auto description = resolve_property_description(meta);
                if (!description.empty()) {
                    label += " - " + description;
                }
                if (ImGui::Selectable(label.c_str(), selected)) {
                    selected_property_id_ = property_id;
                    property_res_id_.clear();
                    refresh_property_value(entry.connection);
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndListBox();
        }
    }
    ImGui::EndChild();
    ImGui::SameLine();
    if (ImGui::BeginChild("property-details-pane", ImVec2(0, 0), true)) {
        if (selected_property_id_.empty()) {
            ImGui::TextUnformatted("Select a property to inspect.");
        } else {
            ImGui::InputText("Resource ID", &property_res_id_);
            ImGui::InputText("Encoding", &property_encoding_);
            ImGui::InputInt("Paginate Offset", &paginate_offset_);
            ImGui::InputInt("Paginate Limit", &paginate_limit_);
            paginate_offset_ = std::max(0, paginate_offset_);
            paginate_limit_ = std::max(1, paginate_limit_);

            if (ImGui::Button("Get Property")) {
                entry.connection->get_property_data(
                    selected_property_id_,
                    property_res_id_,
                    property_encoding_,
                    paginate_offset_,
                    paginate_limit_);
            }
            ImGui::SameLine();
            bool subscribed = has_property_subscription(entry.connection, selected_property_id_);
            if (ImGui::Button(subscribed ? "Unsubscribe" : "Subscribe")) {
                if (subscribed) {
                    entry.connection->unsubscribe_property(selected_property_id_, property_res_id_);
                } else {
                    entry.connection->subscribe_property(selected_property_id_, property_res_id_, property_encoding_);
                }
            }

            ImGui::Separator();
            ImGui::Checkbox("Edit Mode", &edit_mode_);
            if (ImGui::Button("Refresh Local Value")) {
                refresh_property_value(entry.connection);
            }
            ImGui::SameLine();
            if (ImGui::Button("Commit Changes")) {
                if (edit_mode_) {
                    std::vector<uint8_t> data(property_value_buffer_.begin(), property_value_buffer_.end());
                    entry.connection->set_property_data(
                        selected_property_id_,
                        property_res_id_,
                        data,
                        property_encoding_,
                        false);
                }
            }

            ImGuiInputTextFlags flags = edit_mode_ ? ImGuiInputTextFlags_None : ImGuiInputTextFlags_ReadOnly;
            flags |= ImGuiInputTextFlags_AllowTabInput;
            ImGui::InputTextMultiline("Property Value",
                                      &property_value_buffer_,
                                      ImVec2(-FLT_MIN, 180.0f),
                                      flags);
        }
    }
    ImGui::EndChild();
}

void InspectorPanel::render_process_inquiry(const ConnectionEntry& entry) {
    ImGui::TextUnformatted("Process Inquiry");
    if (!entry.connection) {
        ImGui::TextUnformatted("No active connection.");
        return;
    }

    const char* address_labels[] = {"Function Block (7F)", "Group (7E)"};
    uint8_t address_values[] = {0x7F, 0x7E};
    int current_index = (midi_report_address_ == 0x7E) ? 1 : 0;
    if (ImGui::BeginCombo("Address", address_labels[current_index])) {
        for (int i = 0; i < 2; ++i) {
            bool selected = (current_index == i);
            if (ImGui::Selectable(address_labels[i], selected)) {
                midi_report_address_ = address_values[i];
            }
            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    if (ImGui::Button("Request MIDI Message Report")) {
        entry.connection->request_midi_message_report(midi_report_address_, entry.muid);
    }
}

void InspectorPanel::refresh_property_value(const std::shared_ptr<tooling::ClientConnectionModel>& connection) {
    property_value_buffer_.clear();
    if (!connection || selected_property_id_.empty()) {
        return;
    }
    auto values = connection->get_properties().to_vector();
    for (const auto& value : values) {
        if (value.id == selected_property_id_) {
            property_value_buffer_.assign(value.body.begin(), value.body.end());
            return;
        }
    }
}

bool InspectorPanel::has_property_subscription(
    const std::shared_ptr<tooling::ClientConnectionModel>& connection,
    const std::string& property_id) const {
    if (!connection || property_id.empty()) {
        return false;
    }
    auto subs = connection->get_subscriptions().to_vector();
    for (const auto& sub : subs) {
        if (sub.property_id == property_id &&
            sub.state == tooling::SubscriptionState::State::Subscribed) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<tooling::CIDeviceModel> InspectorPanel::device_model() {
    auto model = device_model_.lock();
    if (!model && repository_) {
        if (auto manager = repository_->get_ci_device_manager()) {
            model = manager->get_device_model();
            if (model) {
                device_model_ = model;
                attach_model_callbacks(model);
            }
        }
    }
    return model;
}

void InspectorPanel::attach_model_callbacks(const std::shared_ptr<tooling::CIDeviceModel>& model) {
    connections_dirty_.store(true, std::memory_order_relaxed);
    profiles_dirty_.store(true, std::memory_order_relaxed);
    properties_dirty_.store(true, std::memory_order_relaxed);
    model->add_connections_changed_callback([this]() {
        connections_dirty_.store(true, std::memory_order_relaxed);
        profiles_dirty_.store(true, std::memory_order_relaxed);
        properties_dirty_.store(true, std::memory_order_relaxed);
    });
    model->add_profiles_updated_callback([this]() {
        profiles_dirty_.store(true, std::memory_order_relaxed);
    });
    model->add_properties_updated_callback([this]() {
        properties_dirty_.store(true, std::memory_order_relaxed);
    });
}

void InspectorPanel::ensure_connection_selection(size_t count) {
    if (count == 0) {
        selected_connection_index_ = -1;
        return;
    }
    if (selected_connection_index_ < 0 ||
        selected_connection_index_ >= static_cast<int>(count)) {
        selected_connection_index_ = 0;
        selected_property_id_.clear();
        property_value_buffer_.clear();
    }
}

} // namespace midicci::app
