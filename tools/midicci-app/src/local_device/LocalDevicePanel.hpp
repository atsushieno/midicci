#pragma once

#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include <midicci/details/PropertyHostFacade.hpp>
#include <midicci/details/commonproperties/CommonRulesPropertyMetadata.hpp>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>
#include <memory>
#include <string>
#include <vector>

namespace midicci::app {

class LocalDevicePanel {
public:
    explicit LocalDevicePanel(tooling::CIToolRepository* repository);

    void render();

private:
    struct ProfileEntry {
        std::string profile_id;
    };

    tooling::CIToolRepository* repository_;
    bool device_config_loaded_ = false;

    // Device configuration fields
    std::string manufacturer_id_hex_;
    std::string family_id_hex_;
    std::string model_id_hex_;
    std::string version_id_hex_;
    std::string manufacturer_text_;
    std::string family_text_;
    std::string model_text_;
    std::string version_text_;
    std::string serial_number_;
    std::string product_instance_id_;
    int max_connections_ = 8;
    bool workaround_subscription_ = false;
    bool workaround_profile_channels_ = false;

    // Profile management
    std::string selected_profile_id_;
    std::string new_profile_id_input_;
    int new_profile_address_ = 127;
    int new_profile_channels_ = 1;

    // Property management
    std::string selected_property_id_;
    std::string property_value_buffer_;
    std::string property_res_id_;
    std::string property_media_types_;
    std::string property_encodings_;
    std::string property_schema_;
    bool property_can_get_ = true;
    bool property_can_subscribe_ = true;
    bool property_require_res_id_ = false;
    bool property_can_paginate_ = false;
    std::string property_can_set_ = "full";
    bool property_edit_mode_ = false;

    void ensure_device_config_loaded(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void render_device_configuration(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void render_profiles_section(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void render_properties_section(const std::shared_ptr<tooling::CIDeviceModel>& device_model);

    void load_fields_from_config(midicci::MidiCIDeviceConfiguration& config);
    void apply_device_info(midicci::MidiCIDeviceConfiguration& config);

    std::vector<std::string> gather_profile_ids(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void add_profile(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void add_profile_target(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void render_profile_targets(const std::shared_ptr<tooling::CIDeviceModel>& device_model);

    void add_property(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void delete_property(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void refresh_property_value(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void save_property_value(const std::shared_ptr<tooling::CIDeviceModel>& device_model);
    void save_property_metadata(const std::shared_ptr<tooling::CIDeviceModel>& device_model);

    uint32_t parse_hex(const std::string& value, uint32_t default_value = 0) const;
};

} // namespace midicci::app
