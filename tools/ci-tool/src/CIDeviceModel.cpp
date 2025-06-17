#include "CIDeviceModel.hpp"
#include "CIDeviceManager.hpp"
#include "midi-ci/properties/PropertyManager.hpp"
#include <mutex>
#include <iostream>

namespace ci_tool {

class CIDeviceModel::Impl {
public:
    explicit Impl(CIDeviceManager& parent, uint32_t muid,
                  CIOutputSender ci_sender, MidiMessageReportSender mmr_sender,
                  std::function<void(const std::string&, bool)> logger)
        : parent_(parent), muid_(muid),
          ci_output_sender_(ci_sender), midi_message_report_sender_(mmr_sender),
          logger_(logger),
          receiving_midi_message_reports_(false), last_chunked_message_channel_(0) {}
    
    CIDeviceManager& parent_;
    uint32_t muid_;
    CIOutputSender ci_output_sender_;
    MidiMessageReportSender midi_message_report_sender_;
    std::function<void(const std::string&, bool)> logger_;
    
    std::shared_ptr<midi_ci::core::MidiCIDevice> device_;
    MutableStateList<std::shared_ptr<ClientConnectionModel>> connections_;
    MutableStateList<std::shared_ptr<MidiCIProfileState>> local_profile_states_;
    
    std::vector<ConnectionsChangedCallback> connections_changed_callbacks_;
    std::vector<ProfilesUpdatedCallback> profiles_updated_callbacks_;
    std::vector<PropertiesUpdatedCallback> properties_updated_callbacks_;
    
    bool receiving_midi_message_reports_;
    uint8_t last_chunked_message_channel_;
    std::vector<uint8_t> chunked_messages_;
    
    mutable std::recursive_mutex mutex_{};
};

CIDeviceModel::CIDeviceModel(CIDeviceManager& parent, uint32_t muid,
                           CIOutputSender ci_output_sender,
                           MidiMessageReportSender midi_message_report_sender,
                           std::function<void(const std::string&, bool)> logger)
    : pimpl_(std::make_unique<Impl>(parent, muid, ci_output_sender, midi_message_report_sender, logger)) {
    receiving_midi_message_reports = pimpl_->receiving_midi_message_reports_;
    last_chunked_message_channel = pimpl_->last_chunked_message_channel_;
    chunked_messages = pimpl_->chunked_messages_;
}

CIDeviceModel::~CIDeviceModel() = default;

void CIDeviceModel::initialize() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->device_ = std::make_shared<midi_ci::core::MidiCIDevice>(pimpl_->muid_, pimpl_->logger_);
    pimpl_->device_->set_sysex_sender(pimpl_->ci_output_sender_);
    pimpl_->device_->initialize();
    
    setup_event_listeners();
    add_test_profile_items();
    
    std::cout << "CIDeviceModel initialized with MUID: 0x" << std::hex << pimpl_->muid_ << std::dec << std::endl;
}

void CIDeviceModel::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_->shutdown();
        pimpl_->device_.reset();
    }
    pimpl_->connections_.clear();
    pimpl_->local_profile_states_.clear();
    std::cout << "CIDeviceModel shutdown" << std::endl;
}

std::shared_ptr<midi_ci::core::MidiCIDevice> CIDeviceModel::get_device() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->device_;
}

void CIDeviceModel::process_ci_message(uint8_t group, const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_->processInput(group, data);
    }
}

const MutableStateList<std::shared_ptr<ClientConnectionModel>>& CIDeviceModel::get_connections() const {
    return pimpl_->connections_;
}

const MutableStateList<std::shared_ptr<MidiCIProfileState>>& CIDeviceModel::get_local_profile_states() const {
    return pimpl_->local_profile_states_;
}

void CIDeviceModel::send_discovery() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_->sendDiscovery();
        std::cout << "Sending discovery inquiry..." << std::endl;
    }
}

void CIDeviceModel::send_profile_details_inquiry(uint8_t address, uint32_t muid, 
                                                const midi_ci::profiles::MidiCIProfileId& profile, uint8_t target) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Sending profile details inquiry to MUID: 0x" << std::hex << muid << std::dec << std::endl;
}

void CIDeviceModel::update_local_profile_target(const std::shared_ptr<MidiCIProfileState>& profile_state,
                                               uint8_t new_address, bool enabled, uint16_t num_channels_requested) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (profile_state) {
        profile_state->address().set(new_address);
        profile_state->enabled().set(enabled);
        profile_state->num_channels_requested().set(num_channels_requested);
    }
}

void CIDeviceModel::add_local_profile(const midi_ci::profiles::MidiCIProfile& profile) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    auto profile_state = std::make_shared<MidiCIProfileState>(
        profile.group, profile.address, profile.profile,
        profile.enabled, profile.num_channels_requested);
    pimpl_->local_profile_states_.add(profile_state);
    
    for (const auto& callback : pimpl_->profiles_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::remove_local_profile(uint8_t group, uint8_t address, const midi_ci::profiles::MidiCIProfileId& profile_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->local_profile_states_.remove_if(
        [group, address, &profile_id](const std::shared_ptr<MidiCIProfileState>& state) {
            return state->group().get() == group && 
                   state->address().get() == address && 
                   state->get_profile() == profile_id;
        });
    
    for (const auto& callback : pimpl_->profiles_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::add_local_property(const midi_ci::properties::PropertyMetadata& property) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Added local property: " << property.property_id << std::endl;
    
    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::remove_local_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Removed local property: " << property_id << std::endl;
    
    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::update_property_value(const std::string& property_id, const std::string& res_id, 
                                         const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Updated property: " << property_id << " (resource: " << res_id << ")" << std::endl;
    
    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::setup_event_listeners() {
    if (!pimpl_->device_) return;
    
    pimpl_->device_->set_connections_changed_callback([this]() {
        on_connections_changed();
    });
}

void CIDeviceModel::on_connections_changed() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->device_) return;
    
    const auto& device_connections = pimpl_->device_->get_connections();
    
    std::vector<uint8_t> current_muids;
    for (const auto& [muid, conn] : device_connections) {
        current_muids.push_back(muid);
    }
    
    std::vector<uint8_t> existing_muids;
    auto connections_vec = pimpl_->connections_.to_vector();
    for (const auto& conn_model : connections_vec) {
        if (conn_model && conn_model->get_connection()) {
            existing_muids.push_back(conn_model->get_connection()->get_target_muid());
        }
    }
    
    for (uint8_t muid : current_muids) {
        if (std::find(existing_muids.begin(), existing_muids.end(), muid) == existing_muids.end()) {
            auto device_conn = pimpl_->device_->get_connection(muid);
            if (device_conn) {
                auto conn_model = std::make_shared<ClientConnectionModel>(shared_from_this(), device_conn);
                pimpl_->connections_.add(conn_model);
                std::cout << "Added connection for MUID: 0x" << std::hex << static_cast<uint32_t>(muid) << std::dec << std::endl;
            }
        }
    }
    
    pimpl_->connections_.remove_if(
        [&current_muids](const std::shared_ptr<ClientConnectionModel>& conn_model) {
            if (!conn_model || !conn_model->get_connection()) return true;
            uint8_t muid = conn_model->get_connection()->get_target_muid();
            bool should_remove = std::find(current_muids.begin(), current_muids.end(), muid) == current_muids.end();
            if (should_remove) {
                std::cout << "Removed connection for MUID: 0x" << std::hex << static_cast<uint32_t>(muid) << std::dec << std::endl;
            }
            return should_remove;
        });
    
    for (const auto& callback : pimpl_->connections_changed_callbacks_) {
        callback();
    }
}

void CIDeviceModel::add_test_profile_items() {
    midi_ci::profiles::MidiCIProfileId test_profile({0x7E, 0x00, 0x01, 0x02, 0x03});
    midi_ci::profiles::MidiCIProfile profile(test_profile, 0, 0x7F, false, 1);
    add_local_profile(profile);
    
    std::cout << "Added test profile items" << std::endl;
}

void CIDeviceModel::add_connections_changed_callback(ConnectionsChangedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_changed_callbacks_.push_back(callback);
}

void CIDeviceModel::add_profiles_updated_callback(ProfilesUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_updated_callbacks_.push_back(callback);
}

void CIDeviceModel::add_properties_updated_callback(PropertiesUpdatedCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->properties_updated_callbacks_.push_back(callback);
}

void CIDeviceModel::remove_connections_changed_callback(const ConnectionsChangedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connections_changed_callbacks_.erase(
        std::remove_if(pimpl_->connections_changed_callbacks_.begin(), 
                      pimpl_->connections_changed_callbacks_.end(),
                      [&callback](const ConnectionsChangedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->connections_changed_callbacks_.end());
}

void CIDeviceModel::remove_profiles_updated_callback(const ProfilesUpdatedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_updated_callbacks_.erase(
        std::remove_if(pimpl_->profiles_updated_callbacks_.begin(), 
                      pimpl_->profiles_updated_callbacks_.end(),
                      [&callback](const ProfilesUpdatedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->profiles_updated_callbacks_.end());
}

void CIDeviceModel::remove_properties_updated_callback(const PropertiesUpdatedCallback& callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->properties_updated_callbacks_.erase(
        std::remove_if(pimpl_->properties_updated_callbacks_.begin(), 
                      pimpl_->properties_updated_callbacks_.end(),
                      [&callback](const PropertiesUpdatedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->properties_updated_callbacks_.end());
}

} // namespace ci_tool
