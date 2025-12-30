#pragma once

#include <midicci/tooling/CIToolRepository.hpp>
#include <midicci/tooling/ClientConnectionModel.hpp>
#include <midicci/tooling/MidiCIProfileState.hpp>
#include <midicci/tooling/CIDeviceModel.hpp>
#include <imgui.h>
#include <memory>
#include <string>
#include <vector>
#include <atomic>

namespace midicci::app {

class InspectorPanel {
public:
    explicit InspectorPanel(tooling::CIToolRepository* repository);

    void render();

private:
    struct ConnectionEntry {
        uint32_t muid = 0;
        std::string label;
        std::shared_ptr<tooling::ClientConnectionModel> connection;
    };

    void render_discovery_section(const std::vector<ConnectionEntry>& connections);
    void render_device_details(const ConnectionEntry& entry);
    void render_profiles(const ConnectionEntry& entry);
    void render_properties(const ConnectionEntry& entry);
    void render_process_inquiry(const ConnectionEntry& entry);

    void refresh_property_value(const std::shared_ptr<tooling::ClientConnectionModel>& connection);
    bool has_property_subscription(const std::shared_ptr<tooling::ClientConnectionModel>& connection,
                                   const std::string& property_id) const;

    tooling::CIToolRepository* repository_;
    std::weak_ptr<tooling::CIDeviceModel> device_model_;
    std::atomic<bool> connections_dirty_{true};
    std::atomic<bool> profiles_dirty_{true};
    std::atomic<bool> properties_dirty_{true};

    int selected_connection_index_ = -1;
    std::string selected_property_id_;
    std::string property_value_buffer_;
    std::string property_res_id_;
    std::string property_encoding_;
    int paginate_offset_ = 0;
    int paginate_limit_ = 1024;
    bool edit_mode_ = false;
    uint8_t midi_report_address_ = 0x7F;

    std::shared_ptr<tooling::CIDeviceModel> device_model();
    void attach_model_callbacks(const std::shared_ptr<tooling::CIDeviceModel>& model);
    void ensure_connection_selection(size_t count);
};

} // namespace midicci::app
