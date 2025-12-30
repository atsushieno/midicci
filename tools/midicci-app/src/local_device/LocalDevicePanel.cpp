#include "local_device/LocalDevicePanel.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace midicci::app {
namespace {
constexpr const char* kSystemPropertyDeviceInfo = "DeviceInfo";
constexpr const char* kSystemPropertyChannelList = "ChannelList";

bool parse_profile_id_string(const std::string& text, std::vector<uint8_t>& out) {
    out.clear();
    std::string current;
    for (char ch : text) {
        if (ch == ':' || ch == '-' || ch == ' ') {
            if (!current.empty()) {
                if (current.size() != 2) {
                    return false;
                }
                uint8_t value = static_cast<uint8_t>(std::strtoul(current.c_str(), nullptr, 16));
                out.push_back(value);
                current.clear();
            }
        } else {
            if (!std::isxdigit(static_cast<unsigned char>(ch))) {
                return false;
            }
            current.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(ch))));
        }
    }
    if (!current.empty()) {
        if (current.size() != 2) {
            return false;
        }
        uint8_t value = static_cast<uint8_t>(std::strtoul(current.c_str(), nullptr, 16));
        out.push_back(value);
    }
    return out.size() == 5;
}

bool is_system_property(const std::string& property_id) {
    return property_id == kSystemPropertyDeviceInfo ||
           property_id == kSystemPropertyChannelList;
}

std::string to_hex(uint32_t value, size_t width) {
    std::ostringstream oss;
    oss << std::uppercase << std::setfill('0') << std::setw(static_cast<int>(width)) << std::hex << value;
    return oss.str();
}

std::string resolve_property_description(const midicci::commonproperties::PropertyMetadata* metadata) {
    if (!metadata) {
        return {};
    }
    auto desc = metadata->getExtra("description");
    if (!desc.empty()) {
        return desc;
    }
    return metadata->getExtra("title");
}

std::string property_label(const midicci::commonproperties::PropertyMetadata* metadata) {
    if (!metadata) {
        return {};
    }
    std::string id = metadata->getPropertyId();
    auto desc = resolve_property_description(metadata);
    if (!desc.empty()) {
        id += " - " + desc;
    }
    return id;
}
} // namespace

LocalDevicePanel::LocalDevicePanel(tooling::CIToolRepository* repository)
    : repository_(repository) {}

void LocalDevicePanel::render() {
    if (!repository_) {
        ImGui::TextUnformatted("CIToolRepository unavailable.");
        return;
    }

    auto ci_manager = repository_->get_ci_device_manager();
    auto device_model = ci_manager ? ci_manager->get_device_model() : nullptr;
    if (!device_model || !device_model->get_device()) {
        ImGui::TextUnformatted("Local device not initialized.");
        return;
    }

    ensure_device_config_loaded(device_model);

    render_device_configuration(device_model);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    render_profiles_section(device_model);
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
    render_properties_section(device_model);
}

void LocalDevicePanel::ensure_device_config_loaded(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (device_config_loaded_) {
        return;
    }
    auto device = device_model->get_device();
    if (!device) {
        return;
    }
    load_fields_from_config(device->get_config());
    device_config_loaded_ = true;
}

void LocalDevicePanel::render_device_configuration(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    auto device = device_model->get_device();
    auto& config = device->get_config();

    ImGui::TextUnformatted("Local Device Configuration");
    ImGui::TextWrapped("Note that each ID byte is in 7 bits. Hex values above 0x80 per byte are invalid.");
    ImGui::Spacing();

    auto render_dual_field = [](const char* left_label,
                                const char* left_id,
                                std::string& left_value,
                                const char* right_label,
                                const char* right_id,
                                std::string& right_value) {
        ImGui::TextUnformatted(left_label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText(left_id, &left_value);
        ImGui::NextColumn();
        ImGui::TextUnformatted(right_label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText(right_id, &right_value);
        ImGui::NextColumn();
    };

    auto render_single_field = [](const char* label,
                                  const char* input_id,
                                  std::string& value) {
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputText(input_id, &value);
        ImGui::NextColumn();
        ImGui::Spacing();
        ImGui::NextColumn();
    };

    auto render_single_int = [](const char* label,
                                const char* input_id,
                                int& value,
                                int min_value) {
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(-FLT_MIN);
        ImGui::InputInt(input_id, &value);
        if (value < min_value) {
            value = min_value;
        }
        ImGui::NextColumn();
        ImGui::Spacing();
        ImGui::NextColumn();
    };

    ImGui::Columns(2, "local-device-config-columns", false);
    render_dual_field("Manufacturer ID (hex)", "##manu_id", manufacturer_id_hex_,
                      "Text", "##manu_text", manufacturer_text_);
    render_dual_field("Family ID (hex)", "##family_id", family_id_hex_,
                      "Text", "##family_text", family_text_);
    render_dual_field("Model ID (hex)", "##model_id", model_id_hex_,
                      "Text", "##model_text", model_text_);
    render_dual_field("Version ID (hex)", "##version_id", version_id_hex_,
                      "Text", "##version_text", version_text_);
    render_single_field("Serial Number", "##serial_text", serial_number_);
    render_single_field("Product Instance ID", "##prod_instance", product_instance_id_);
    render_single_int("Max Connections", "##max_conn", max_connections_, 1);
    ImGui::Columns(1);

    if (ImGui::Button("Apply Device Info")) {
        apply_device_info(config);
        repository_->log("Updated local device information", tooling::MessageDirection::Out);
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset to Current")) {
        load_fields_from_config(config);
    }

    ImGui::Checkbox("Workaround JUCE Subscription", &workaround_subscription_);
    ImGui::SameLine();
    ImGui::Checkbox("Workaround JUCE Profile Channels", &workaround_profile_channels_);
    ImGui::Spacing();
}

void LocalDevicePanel::render_profiles_section(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    ImGui::TextUnformatted("Local Profiles");

    auto profile_ids = gather_profile_ids(device_model);
    if (selected_profile_id_.empty() && !profile_ids.empty()) {
        selected_profile_id_ = profile_ids.front();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float total_width = avail.x;
    float list_width = std::max(0.0f, total_width * 0.3f);
    float profile_section_height = std::clamp(avail.y * 0.4f, 220.0f, 360.0f);

    if (ImGui::BeginChild("local-profiles-list", ImVec2(list_width, profile_section_height), true)) {
        if (ImGui::BeginListBox("Profiles", ImVec2(-FLT_MIN, -FLT_MIN))) {
            for (const auto& pid : profile_ids) {
                bool selected = (pid == selected_profile_id_);
                if (ImGui::Selectable(pid.c_str(), selected)) {
                    selected_profile_id_ = pid;
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
    if (ImGui::BeginChild("local-profiles-details", ImVec2(0, profile_section_height), true)) {
        ImGui::TextUnformatted("Profile Setup");
        ImGui::InputText("New Profile ID (XX:..)", &new_profile_id_input_);
        ImGui::InputInt("Target Address", &new_profile_address_);
        new_profile_address_ = std::clamp(new_profile_address_, 0, 127);
        ImGui::InputInt("Channels", &new_profile_channels_);
        new_profile_channels_ = std::clamp(new_profile_channels_, 1, 16);

        if (ImGui::Button("Add Profile")) {
            add_profile(device_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Target")) {
            add_profile_target(device_model);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Test Items")) {
            device_model->add_test_profile_items();
            repository_->log("Added test profile items", tooling::MessageDirection::Out);
        }

        ImGui::Separator();
        render_profile_targets(device_model);
    }
    ImGui::EndChild();
}

void LocalDevicePanel::render_properties_section(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    auto device = device_model->get_device();
    auto& property_facade = device->get_property_host_facade();

    ImGui::TextUnformatted("Local Properties");

    std::vector<std::string> property_ids = {
        kSystemPropertyDeviceInfo,
        kSystemPropertyChannelList
    };
    auto metadata_list = property_facade.get_metadata_list();
    for (const auto* metadata : metadata_list) {
        if (metadata) {
            property_ids.push_back(metadata->getPropertyId());
        }
    }

    if (selected_property_id_.empty() && !property_ids.empty()) {
        selected_property_id_ = property_ids.front();
    }

    ImVec2 avail = ImGui::GetContentRegionAvail();
    float total_width = avail.x;
    float list_width = std::max(0.0f, total_width * 0.25f);
    float properties_section_height = std::clamp(avail.y * 0.85f, 320.0f, 560.0f);

    if (ImGui::BeginChild("local-props-list", ImVec2(list_width, properties_section_height), true)) {
        if (property_ids.empty()) {
            ImGui::TextUnformatted("No properties available.");
        } else if (ImGui::BeginListBox("Property Catalog", ImVec2(-FLT_MIN, -FLT_MIN))) {
            for (const auto& prop : property_ids) {
                bool selected = (prop == selected_property_id_);
                std::string label = prop;
                const auto* meta = property_facade.get_property_metadata(prop);
                if (meta) {
                    auto desc = resolve_property_description(meta);
                    if (!desc.empty()) {
                        label += " - " + desc;
                    }
                }
                if (ImGui::Selectable(label.c_str(), selected)) {
                    selected_property_id_ = prop;
                    refresh_property_value(device_model);
                    const auto* meta = property_facade.get_property_metadata(selected_property_id_);
                    if (auto* rules = dynamic_cast<const midicci::commonproperties::CommonRulesPropertyMetadata*>(meta)) {
                        property_can_get_ = rules->canGet;
                        property_can_set_ = rules->canSet;
                        property_can_subscribe_ = rules->canSubscribe;
                        property_require_res_id_ = rules->requireResId;
                        property_can_paginate_ = rules->canPaginate;
                        property_media_types_.clear();
                        for (size_t i = 0; i < rules->mediaTypes.size(); ++i) {
                            if (i > 0) property_media_types_ += ", ";
                            property_media_types_ += rules->mediaTypes[i];
                        }
                        property_encodings_.clear();
                        for (size_t i = 0; i < rules->encodings.size(); ++i) {
                            if (i > 0) property_encodings_ += ", ";
                            property_encodings_ += rules->encodings[i];
                        }
                        property_schema_ = rules->schema;
                    }
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
    if (ImGui::BeginChild("local-props-details", ImVec2(0, properties_section_height), true)) {
        if (selected_property_id_.empty()) {
            ImGui::TextUnformatted("Select a property to edit.");
        } else {
            if (ImGui::Button("Add Property")) {
                add_property(device_model);
            }
            ImGui::SameLine();
            bool is_system = is_system_property(selected_property_id_);
            if (ImGui::Button("Delete Property")) {
                if (!is_system) {
                    delete_property(device_model);
                }
            }
            if (is_system) {
                ImGui::SameLine();
                ImGui::TextUnformatted("(system property)");
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Metadata");
            ImGui::Checkbox("Can Get", &property_can_get_);
            ImGui::SameLine();
            ImGui::Checkbox("Can Subscribe", &property_can_subscribe_);
            ImGui::SameLine();
            ImGui::Checkbox("Require ResId", &property_require_res_id_);
            ImGui::Checkbox("Can Paginate", &property_can_paginate_);
            ImGui::InputText("Can Set", &property_can_set_);
            ImGui::InputText("Media Types", &property_media_types_);
            ImGui::InputText("Encodings", &property_encodings_);
            ImGui::InputTextMultiline("Schema", &property_schema_, ImVec2(-FLT_MIN, 120.0f));
            if (ImGui::Button("Save Metadata")) {
                if (!is_system) {
                    save_property_metadata(device_model);
                }
            }

            ImGui::Separator();
            ImGui::Checkbox("Edit Value", &property_edit_mode_);
            if (ImGui::Button("Refresh Value")) {
                refresh_property_value(device_model);
            }
            ImGui::InputText("Resource ID", &property_res_id_);
            ImGui::InputTextMultiline("Value", &property_value_buffer_, ImVec2(-FLT_MIN, 180.0f),
                                      property_edit_mode_ ? 0 : ImGuiInputTextFlags_ReadOnly);
            if (property_edit_mode_ && ImGui::Button("Apply Value")) {
                save_property_value(device_model);
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Property Subscriptions");
            auto subs = property_facade.get_subscriptions();
            if (subs.empty()) {
                ImGui::TextUnformatted("No active subscriptions.");
            } else if (ImGui::BeginTable("subscriptions", 3, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
                ImGui::TableSetupColumn("MUID");
                ImGui::TableSetupColumn("ResID");
                ImGui::TableSetupColumn("Subscription ID");
                ImGui::TableHeadersRow();
                for (const auto& sub : subs) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::Text("0x%08X", sub.subscriber_muid);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::TextUnformatted(sub.res_id.c_str());
                    ImGui::TableSetColumnIndex(2);
                    ImGui::TextUnformatted(sub.subscription_id.c_str());
                }
                ImGui::EndTable();
            }
        }
    }
    ImGui::EndChild();
}

void LocalDevicePanel::load_fields_from_config(midicci::MidiCIDeviceConfiguration& config) {
    manufacturer_id_hex_ = to_hex(config.device_info.manufacturer_id, 6);
    family_id_hex_ = to_hex(config.device_info.family_id, 4);
    model_id_hex_ = to_hex(config.device_info.model_id, 4);
    version_id_hex_ = to_hex(config.device_info.version_id, 8);
    manufacturer_text_ = config.device_info.manufacturer;
    family_text_ = config.device_info.family;
    model_text_ = config.device_info.model;
    version_text_ = config.device_info.version;
    serial_number_ = config.device_info.serial_number;
    product_instance_id_ = config.product_instance_id;
}

void LocalDevicePanel::apply_device_info(midicci::MidiCIDeviceConfiguration& config) {
    config.device_info.manufacturer_id = parse_hex(manufacturer_id_hex_, config.device_info.manufacturer_id);
    config.device_info.family_id = static_cast<uint16_t>(parse_hex(family_id_hex_, config.device_info.family_id));
    config.device_info.model_id = static_cast<uint16_t>(parse_hex(model_id_hex_, config.device_info.model_id));
    config.device_info.version_id = parse_hex(version_id_hex_, config.device_info.version_id);
    config.device_info.manufacturer = manufacturer_text_;
    config.device_info.family = family_text_;
    config.device_info.model = model_text_;
    config.device_info.version = version_text_;
    config.device_info.serial_number = serial_number_;
    config.product_instance_id = product_instance_id_;
}

std::vector<std::string> LocalDevicePanel::gather_profile_ids(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    std::vector<std::string> ids;
    auto profiles = device_model->get_local_profile_states().to_vector();
    for (const auto& profile_state : profiles) {
        if (!profile_state) {
            continue;
        }
        auto id = profile_state->get_profile().to_string();
        if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
            ids.push_back(id);
        }
    }
    return ids;
}

void LocalDevicePanel::add_profile(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    std::vector<uint8_t> bytes;
    if (!parse_profile_id_string(new_profile_id_input_, bytes)) {
        repository_->log("Invalid profile ID format", tooling::MessageDirection::Out);
        return;
    }
    midicci::MidiCIProfileId profile_id(bytes);
    midicci::MidiCIProfile profile(profile_id, 0, static_cast<uint8_t>(new_profile_address_), false,
                                   static_cast<uint16_t>(new_profile_channels_));
    device_model->add_local_profile(profile);
    repository_->log("Added local profile target", tooling::MessageDirection::Out);
    selected_profile_id_ = profile_id.to_string();
}

void LocalDevicePanel::add_profile_target(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (selected_profile_id_.empty()) {
        repository_->log("Select a profile before adding target", tooling::MessageDirection::Out);
        return;
    }
    std::vector<uint8_t> bytes;
    if (!parse_profile_id_string(selected_profile_id_, bytes)) {
        return;
    }
    midicci::MidiCIProfileId profile_id(bytes);
    midicci::MidiCIProfile profile(profile_id, 0, static_cast<uint8_t>(new_profile_address_), false,
                                   static_cast<uint16_t>(new_profile_channels_));
    device_model->add_local_profile(profile);
    repository_->log("Added target to existing profile", tooling::MessageDirection::Out);
}

void LocalDevicePanel::render_profile_targets(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (selected_profile_id_.empty()) {
        ImGui::TextUnformatted("Select a profile to view targets.");
        return;
    }
    std::vector<uint8_t> bytes;
    if (!parse_profile_id_string(selected_profile_id_, bytes)) {
        ImGui::TextUnformatted("Invalid profile ID.");
        return;
    }
    midicci::MidiCIProfileId profile_id(bytes);
    auto states = device_model->get_local_profile_states().to_vector();

    if (ImGui::BeginTable("profile-targets", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders)) {
        ImGui::TableSetupColumn("Enabled");
        ImGui::TableSetupColumn("Group");
        ImGui::TableSetupColumn("Address");
        ImGui::TableSetupColumn("Channels");
        ImGui::TableSetupColumn("Actions");
        ImGui::TableHeadersRow();

        for (auto& state : states) {
            if (!state || state->get_profile().to_string() != profile_id.to_string()) {
                continue;
            }
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool enabled = state->enabled().get();
            std::string label = "##enabled-" + state->get_profile().to_string() + std::to_string(state->address().get());
            if (ImGui::Checkbox(label.c_str(), &enabled)) {
                device_model->update_local_profile_target(state, state->address().get(), enabled,
                                                          state->num_channels_requested().get());
            }

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%u", state->group().get());
            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%u", state->address().get());
            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%u", state->num_channels_requested().get());
            ImGui::TableSetColumnIndex(4);
            std::string btn = "Delete##" + std::to_string(state->address().get());
            if (ImGui::Button(btn.c_str())) {
                device_model->remove_local_profile(state->group().get(), state->address().get(), state->get_profile());
            }
        }
        ImGui::EndTable();
    }
}

void LocalDevicePanel::add_property(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (!device_model) {
        return;
    }
    auto property = device_model->create_new_property();
    if (!property) {
        return;
    }
    repository_->log("Added local property " + property->getPropertyId(), tooling::MessageDirection::Out);
}

void LocalDevicePanel::delete_property(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (selected_property_id_.empty() || is_system_property(selected_property_id_)) {
        return;
    }
    device_model->remove_local_property(selected_property_id_);
    repository_->log("Deleted local property " + selected_property_id_, tooling::MessageDirection::Out);
    selected_property_id_.clear();
}

void LocalDevicePanel::refresh_property_value(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (selected_property_id_.empty()) {
        property_value_buffer_.clear();
        return;
    }
    auto device = device_model->get_device();
    auto values = device->get_property_host_facade().get_properties().getValues();
    for (const auto& value : values) {
        if (value.id == selected_property_id_) {
            property_value_buffer_.assign(value.body.begin(), value.body.end());
            return;
        }
    }
    property_value_buffer_.clear();
}

void LocalDevicePanel::save_property_value(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    if (selected_property_id_.empty() || is_system_property(selected_property_id_)) {
        return;
    }
    std::vector<uint8_t> bytes(property_value_buffer_.begin(), property_value_buffer_.end());
    device_model->update_property_value(selected_property_id_, property_res_id_, bytes);
    repository_->log("Updated property value " + selected_property_id_, tooling::MessageDirection::Out);
}

void LocalDevicePanel::save_property_metadata(const std::shared_ptr<tooling::CIDeviceModel>& device_model) {
    midicci::commonproperties::CommonRulesPropertyMetadata metadata(selected_property_id_);
    metadata.canGet = property_can_get_;
    metadata.canSet = property_can_set_;
    metadata.canSubscribe = property_can_subscribe_;
    metadata.requireResId = property_require_res_id_;
    metadata.canPaginate = property_can_paginate_;

    metadata.mediaTypes.clear();
    std::stringstream media(property_media_types_);
    std::string entry;
    while (std::getline(media, entry, ',')) {
        if (!entry.empty()) {
            auto trimmed = entry;
            trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char c) { return !std::isspace(c); }));
            trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), trimmed.end());
            if (!trimmed.empty()) {
                metadata.mediaTypes.push_back(trimmed);
            }
        }
    }
    if (metadata.mediaTypes.empty()) {
        metadata.mediaTypes.push_back("application/json");
    }

    metadata.encodings.clear();
    std::stringstream enc(property_encodings_);
    while (std::getline(enc, entry, ',')) {
        if (!entry.empty()) {
            auto trimmed = entry;
            trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char c) { return !std::isspace(c); }));
            trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), trimmed.end());
            if (!trimmed.empty()) {
                metadata.encodings.push_back(trimmed);
            }
        }
    }
    if (metadata.encodings.empty()) {
        metadata.encodings.push_back("ASCII");
    }

    metadata.schema = property_schema_;
    if (metadata.schema.empty()) {
        metadata.schema = "{}";
    }

    device_model->update_property_metadata(selected_property_id_, metadata);
    repository_->log("Updated property metadata " + selected_property_id_, tooling::MessageDirection::Out);
}

uint32_t LocalDevicePanel::parse_hex(const std::string& value, uint32_t default_value) const {
    if (value.empty()) {
        return default_value;
    }
    char* end = nullptr;
    auto parsed = std::strtoul(value.c_str(), &end, 16);
    if (end == value.c_str()) {
        return default_value;
    }
    return static_cast<uint32_t>(parsed);
}

} // namespace midicci::app
