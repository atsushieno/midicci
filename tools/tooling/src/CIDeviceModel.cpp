#include "CIDeviceModel.hpp"
#include "CIDeviceManager.hpp"
#include "midicci/ObservablePropertyList.hpp"
#include "midicci/ProfileHostFacade.hpp"
#include "midicci/PropertyHostFacade.hpp"
#include "midicci/commonproperties/CommonRulesPropertyMetadata.hpp"
#include <mutex>
#include <iostream>

namespace midicci::tooling {

class CIDeviceModel::Impl {
public:
    explicit Impl(CIDeviceManager& parent, MidiCIDeviceConfiguration& config, uint32_t muid,
                  CIOutputSender ci_sender, MidiMessageReportSender mmr_sender,
                  std::function<void(const std::string&, bool)> logger)
        : parent_(parent), config_(config), muid_(muid),
          ci_output_sender_(ci_sender), midi_message_report_sender_(mmr_sender),
          logger_(logger),
          receiving_midi_message_reports_(false), last_chunked_message_channel_(0) {}
    
    CIDeviceManager& parent_;
    MidiCIDeviceConfiguration& config_;
    uint32_t muid_;
    CIOutputSender ci_output_sender_;
    MidiMessageReportSender midi_message_report_sender_;
    std::function<void(const std::string&, bool)> logger_;
    
    std::shared_ptr<MidiCIDevice> device_;
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

CIDeviceModel::CIDeviceModel(CIDeviceManager& parent, MidiCIDeviceConfiguration& config,
                             uint32_t muid, CIOutputSender ci_output_sender,
                             MidiMessageReportSender midi_message_report_sender,
                             std::function<void(const std::string&, bool)> logger)
    : pimpl_(std::make_unique<Impl>(parent, config, muid, ci_output_sender, midi_message_report_sender, logger)) {
    receiving_midi_message_reports = pimpl_->receiving_midi_message_reports_;
    last_chunked_message_channel = pimpl_->last_chunked_message_channel_;
    chunked_messages = pimpl_->chunked_messages_;
}

CIDeviceModel::~CIDeviceModel() = default;

void CIDeviceModel::initialize() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    pimpl_->device_ = std::make_shared<MidiCIDevice>(pimpl_->muid_, pimpl_->config_, pimpl_->logger_);
    pimpl_->device_->set_sysex_sender(pimpl_->ci_output_sender_);

    setup_event_listeners();

    std::cout << "CIDeviceModel initialized with MUID: 0x" << std::hex << pimpl_->muid_ << std::dec << std::endl;
}

void CIDeviceModel::shutdown() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->device_) {
        pimpl_->device_.reset();
    }
    pimpl_->connections_.clear();
    pimpl_->local_profile_states_.clear();
    std::cout << "CIDeviceModel shutdown" << std::endl;
}

std::shared_ptr<MidiCIDevice> CIDeviceModel::get_device() const {
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

MutableStateList<std::shared_ptr<MidiCIProfileState>>& CIDeviceModel::get_local_profile_states() const {
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
                                                 const MidiCIProfileId& profile, uint8_t target) {
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

void CIDeviceModel::add_local_profile(const MidiCIProfile& profile) {
    get_device()->get_profile_host_facade().add_profile(profile);
}

void CIDeviceModel::remove_local_profile(uint8_t group, uint8_t address, const MidiCIProfileId& profile_id) {
    get_device()->get_profile_host_facade().remove_profile(group, address, profile_id);
}

void CIDeviceModel::remove_local_property(const std::string& property_id) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Removed local property: " << property_id << std::endl;
    
    // Actually remove from the PropertyHostFacade using new API
    if (pimpl_->device_) {
        auto& property_facade = pimpl_->device_->get_property_host_facade();
        property_facade.removeProperty(property_id);
    }

    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

void CIDeviceModel::update_property_value(const std::string& property_id, const std::string& res_id,
                                         const std::vector<uint8_t>& data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Updated property: " << property_id << " (resource: " << res_id << ")" << std::endl;
    
    // Actually update the PropertyHostFacade using new API
    if (pimpl_->device_) {
        auto& property_facade = pimpl_->device_->get_property_host_facade();
        property_facade.setPropertyValue(property_id, res_id, data, false); // false = not partial
    }
    
    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

std::vector<std::string> CIDeviceModel::get_local_property_ids() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->device_) {
        auto& property_facade = pimpl_->device_->get_property_host_facade();
        return property_facade.get_property_ids();
    }
    
    return {};
}

void CIDeviceModel::update_property_metadata(const std::string& property_id, const midicci::commonproperties::PropertyMetadata& metadata) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    std::cout << "Updated property metadata: " << property_id << std::endl;
    
    // Actually update the PropertyHostFacade using new API
    if (pimpl_->device_) {
        auto& property_facade = pimpl_->device_->get_property_host_facade();
        property_facade.updatePropertyMetadata(property_id, metadata);
    }
    
    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
        callback();
    }
}

const midicci::commonproperties::PropertyMetadata* CIDeviceModel::get_local_property_metadata(const std::string& property_id) const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    
    if (pimpl_->device_) {
        auto& property_facade = pimpl_->device_->get_property_host_facade();
        return property_facade.get_property_metadata(property_id);
    }
    
    return nullptr;
}

void CIDeviceModel::setup_event_listeners() {
    if (!pimpl_->device_) return;
    
    pimpl_->device_->set_connections_changed_callback([this]() {
        on_connections_changed();
    });
    
    auto& profile_facade = pimpl_->device_->get_profile_host_facade();
    auto& observable_profiles = profile_facade.get_profiles();
    
    observable_profiles.add_profiles_changed_callback([this](auto change, const auto& profile) {
        std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
        
        if (change == ObservableProfileList::ProfilesChange::Added) {
            auto profile_state = std::make_shared<MidiCIProfileState>(
                profile.group, profile.address, profile.profile, 
                profile.enabled, profile.num_channels_requested);
            pimpl_->local_profile_states_.add(profile_state);
        } else if (change == ObservableProfileList::ProfilesChange::Removed) {
            pimpl_->local_profile_states_.remove_if([&profile](const auto& state) {
                return state && state->get_profile().to_string() == profile.profile.to_string() &&
                       state->group().get() == profile.group && 
                       state->address().get() == profile.address;
            });
        }
        

    });
    
    observable_profiles.add_profile_enabled_changed_callback([this](const auto& profile) {
        std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
        
        auto states = pimpl_->local_profile_states_.to_vector();
        for (auto& state : states) {
            if (state && state->get_profile().to_string() == profile.profile.to_string() &&
                state->group().get() == profile.group && 
                state->address().get() == profile.address) {
                state->enabled().set(profile.enabled);
                state->num_channels_requested().set(profile.num_channels_requested);
                break;
            }
        }
        

    });
    
    observable_profiles.add_profile_updated_callback([this](const auto& profile_id, uint8_t old_address, bool enabled, uint8_t new_address, uint16_t num_channels) {
        std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
        
        auto states = pimpl_->local_profile_states_.to_vector();
        for (auto& state : states) {
            if (state && state->get_profile().to_string() == profile_id.to_string() &&
                state->address().get() == old_address) {
                state->address().set(new_address);
                state->enabled().set(enabled);
                state->num_channels_requested().set(num_channels);
                break;
            }
        }
        

    });
    
    // Set up property host facade subscription change callback
    auto& property_facade = pimpl_->device_->get_property_host_facade();
    property_facade.set_subscription_changed_callback([this](const std::string& property_id) {
        std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
        
        // Notify UI that property subscriptions have changed
        for (const auto& callback : pimpl_->properties_updated_callbacks_) {
            callback();
        }
    });
}

void CIDeviceModel::on_connections_changed() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (!pimpl_->device_) return;
    
    const auto& device_connections = pimpl_->device_->get_connections();
    
    std::vector<uint32_t> current_muids;
    for (const auto& [muid, conn] : device_connections) {
        current_muids.push_back(muid);
    }
    
    std::vector<uint32_t> existing_muids;
    auto connections_vec = pimpl_->connections_.to_vector();
    for (const auto& conn_model : connections_vec) {
        if (conn_model && conn_model->get_connection()) {
            existing_muids.push_back(conn_model->get_connection()->get_target_muid());
        }
    }
    
    for (uint32_t muid : current_muids) {
        if (std::find(existing_muids.begin(), existing_muids.end(), muid) == existing_muids.end()) {
            auto device_conn = pimpl_->device_->get_connection(muid);
            if (device_conn) {
                auto conn_model = std::make_shared<ClientConnectionModel>(shared_from_this(), device_conn);
                
                // Set up property update propagation from this connection
                conn_model->add_properties_changed_callback([this]() {
                    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
                    for (const auto& callback : pimpl_->properties_updated_callbacks_) {
                        callback();
                    }
                });
                
                pimpl_->connections_.add(conn_model);
                std::cout << "Added connection for MUID: 0x" << std::hex << static_cast<uint32_t>(muid) << std::dec << std::endl;
            }
        }
    }
    
    pimpl_->connections_.remove_if(
        [&current_muids](const std::shared_ptr<ClientConnectionModel>& conn_model) {
            if (!conn_model || !conn_model->get_connection()) return true;
            uint32_t muid = conn_model->get_connection()->get_target_muid();
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
    std::vector<uint8_t> id1{0x7E, 0x00, 0x01, 0x02, 0x03}, id2{0x7E, 0x05, 0x06, 0x07, 0x08};
    MidiCIProfile profile1{MidiCIProfileId{id1}, 0, 0x7E, true, 0};
    MidiCIProfile profile2{MidiCIProfileId{id2}, 0, 0x7F, true, 0};
    get_device()->get_profile_host_facade().add_profile(profile1);
    get_device()->get_profile_host_facade().add_profile(profile2);
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

    const midicci::commonproperties::PropertyMetadata* CIDeviceModel::create_new_property() {
        auto property = std::make_unique<CommonRulesPropertyMetadata>();
        property->resource = "X-${Random.nextInt(9999)}";

        auto ret = property.get();
        get_device()->get_property_host_facade().addMetadata(std::move(property));
        return ret;
    }

} // namespace ci_tool
