#include "ClientConnectionModel.hpp"
#include "CIDeviceModel.hpp"
#include "midi-ci/properties/ObservablePropertyList.hpp"
#include <mutex>
#include <iostream>

namespace ci_tool {

class ClientConnectionModel::Impl {
public:
    explicit Impl(std::shared_ptr<CIDeviceModel> parent,
                  std::shared_ptr<midi_ci::core::ClientConnection> conn)
        : parent_(parent), connection_(conn) {}
    
    std::shared_ptr<CIDeviceModel> parent_;
    std::shared_ptr<midi_ci::core::ClientConnection> connection_;
    MutableStateList<std::shared_ptr<MidiCIProfileState>> profiles_;
    MutableStateList<SubscriptionState> subscriptions_;
    MutableStateList<midi_ci::properties::PropertyValue> properties_;
    MutableState<std::string> device_info_;
    
    std::vector<ProfilesChangedCallback> profiles_changed_callbacks_;
    std::vector<PropertiesChangedCallback> properties_changed_callbacks_;
    std::vector<DeviceInfoChangedCallback> device_info_changed_callbacks_;
    
    mutable std::mutex mutex_;
};

ClientConnectionModel::ClientConnectionModel(std::shared_ptr<CIDeviceModel> parent,
                                           std::shared_ptr<midi_ci::core::ClientConnection> connection)
    : pimpl_(std::make_unique<Impl>(parent, connection)) {
    setup_profile_listeners();
    setup_property_listeners();
}

ClientConnectionModel::~ClientConnectionModel() = default;

std::shared_ptr<midi_ci::core::ClientConnection> ClientConnectionModel::get_connection() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->connection_;
}

const MutableStateList<std::shared_ptr<MidiCIProfileState>>& ClientConnectionModel::get_profiles() const {
    return pimpl_->profiles_;
}

const MutableStateList<SubscriptionState>& ClientConnectionModel::get_subscriptions() const {
    return pimpl_->subscriptions_;
}

const MutableStateList<midi_ci::properties::PropertyValue>& ClientConnectionModel::get_properties() const {
    return pimpl_->properties_;
}

std::string ClientConnectionModel::get_device_info_value() const {
    return pimpl_->device_info_.get();
}

const MutableState<std::string>& ClientConnectionModel::get_device_info() const {
    return pimpl_->device_info_;
}

void ClientConnectionModel::set_profile(uint8_t group, uint8_t address, const midi_ci::profiles::MidiCIProfileId& profile,
                                       bool new_enabled, uint16_t new_num_channels_requested) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto profiles_vec = pimpl_->profiles_.to_vector();
    auto it = std::find_if(profiles_vec.begin(), profiles_vec.end(),
        [group, address, &profile](const std::shared_ptr<MidiCIProfileState>& state) {
            return state->group().get() == group && 
                   state->address().get() == address && 
                   state->get_profile() == profile;
        });
    
    if (it != profiles_vec.end()) {
        (*it)->enabled().set(new_enabled);
        (*it)->num_channels_requested().set(new_num_channels_requested);
    } else {
        auto new_profile = std::make_shared<MidiCIProfileState>(
            group, address, profile, new_enabled, new_num_channels_requested);
        pimpl_->profiles_.add(new_profile);
    }
    
    std::cout << "Set profile state - Group: " << static_cast<int>(group) 
              << ", Address: " << static_cast<int>(address) 
              << ", Enabled: " << new_enabled << std::endl;
}

std::vector<std::unique_ptr<midi_ci::properties::PropertyMetadata>> ClientConnectionModel::get_metadata_list() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return {};
}



void ClientConnectionModel::get_property_data(const std::string& resource, const std::string& encoding,
                                            int paginate_offset, int paginate_limit) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::cout << "Getting property data for resource: " << resource << std::endl;
}

void ClientConnectionModel::set_property_data(const std::string& resource, const std::string& res_id,
                                            const std::vector<uint8_t>& data, const std::string& encoding,
                                            bool is_partial) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::cout << "Setting property data for resource: " << resource 
              << " (partial: " << is_partial << ")" << std::endl;
}

void ClientConnectionModel::subscribe_property(const std::string& resource, const std::string& mutual_encoding) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto subscriptions_vec = pimpl_->subscriptions_.to_vector();
    auto it = std::find_if(subscriptions_vec.begin(), subscriptions_vec.end(),
        [&resource](const SubscriptionState& sub) {
            return sub.property_id == resource;
        });
    
    if (it == subscriptions_vec.end()) {
        pimpl_->subscriptions_.add(SubscriptionState(resource, SubscriptionState::State::Subscribing));
    }
    
    std::cout << "Subscribing to property: " << resource << std::endl;
}

void ClientConnectionModel::unsubscribe_property(const std::string& resource) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto subscriptions_vec = pimpl_->subscriptions_.to_vector();
    auto it = std::find_if(subscriptions_vec.begin(), subscriptions_vec.end(),
        [&resource](const SubscriptionState& sub) {
            return sub.property_id == resource;
        });
    
    if (it != subscriptions_vec.end()) {
        pimpl_->subscriptions_.remove(*it);
        pimpl_->subscriptions_.add(SubscriptionState(resource, SubscriptionState::State::Unsubscribed));
    }
    
    std::cout << "Unsubscribing from property: " << resource << std::endl;
}

void ClientConnectionModel::request_midi_message_report(uint8_t address, uint32_t target_muid,
                                                       uint8_t message_data_control,
                                                       uint8_t system_messages,
                                                       uint8_t channel_controller_messages,
                                                       uint8_t note_data_messages) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::cout << "Requesting MIDI message report from MUID: 0x" << std::hex << target_muid << std::dec << std::endl;
}

void ClientConnectionModel::setup_profile_listeners() {
    if (!pimpl_->connection_) return;
    
    auto& profile_client = pimpl_->connection_->get_profile_client_facade();
    auto& profiles = profile_client.get_profiles();
    
    profiles.add_profiles_changed_callback([this](auto change, const auto& profile) {
        on_profile_changed();
    });
    
    profiles.add_profile_enabled_changed_callback([this](const auto& profile) {
        on_profile_changed();
    });
}

void ClientConnectionModel::setup_property_listeners() {
    if (!pimpl_->connection_) return;
    
    auto& property_facade = pimpl_->connection_->get_property_client_facade();
    auto* observable_properties = property_facade.get_properties();
    
    if (observable_properties) {
        auto initial_values = observable_properties->getValues();
        for (const auto& value : initial_values) {
            pimpl_->properties_.add(value);
        }
        
        observable_properties->addPropertyUpdatedCallback([this](const std::string& propertyId) {
            if (pimpl_->connection_) {
                auto& property_facade = pimpl_->connection_->get_property_client_facade();
                auto* observable_properties = property_facade.get_properties();
                if (observable_properties) {
                    auto current_values = observable_properties->getValues();
                    auto& properties_list = pimpl_->properties_;
                    
                    auto properties_vec = properties_list.to_vector();
                    auto it = std::find_if(properties_vec.begin(), properties_vec.end(),
                        [&propertyId](const midi_ci::properties::PropertyValue& prop) {
                            return prop.id == propertyId;
                        });
                    
                    auto new_prop_it = std::find_if(current_values.begin(), current_values.end(),
                        [&propertyId](const midi_ci::properties::PropertyValue& prop) {
                            return prop.id == propertyId;
                        });
                    
                    if (new_prop_it != current_values.end()) {
                        if (it != properties_vec.end()) {
                            properties_list.remove(*it);
                        }
                        properties_list.add(*new_prop_it);
                    }
                }
            }
            on_property_value_updated();
        });
        
        observable_properties->addPropertyCatalogUpdatedCallback([this]() {
            if (pimpl_->connection_) {
                auto& property_facade = pimpl_->connection_->get_property_client_facade();
                auto* observable_properties = property_facade.get_properties();
                if (observable_properties) {
                    pimpl_->properties_.clear();
                    auto current_values = observable_properties->getValues();
                    for (const auto& value : current_values) {
                        pimpl_->properties_.add(value);
                    }
                }
            }
            on_property_value_updated();
        });
    }
    
    std::cout << "Set up property listeners for connection" << std::endl;
}

void ClientConnectionModel::on_profile_changed() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    if (!pimpl_->connection_) return;
    
    auto& profile_client = pimpl_->connection_->get_profile_client_facade();
    auto& profiles = profile_client.get_profiles();
    const auto& profile_list = profiles.get_profiles();
    
    pimpl_->profiles_.clear();
    for (const auto& profile : profile_list) {
        auto profile_state = std::make_shared<MidiCIProfileState>(
            profile.group, profile.address, profile.profile, 
            profile.enabled, profile.num_channels_requested);
        pimpl_->profiles_.add(profile_state);
    }
    
    std::cout << "Updated profile list - count: " << pimpl_->profiles_.size() << std::endl;
    
    for (const auto& callback : pimpl_->profiles_changed_callbacks_) {
        callback();
    }
}

void ClientConnectionModel::add_profiles_changed_callback(ProfilesChangedCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_changed_callbacks_.push_back(callback);
}

void ClientConnectionModel::add_properties_changed_callback(PropertiesChangedCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->properties_changed_callbacks_.push_back(callback);
}

void ClientConnectionModel::add_device_info_changed_callback(DeviceInfoChangedCallback callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->device_info_changed_callbacks_.push_back(callback);
}

void ClientConnectionModel::remove_profiles_changed_callback(const ProfilesChangedCallback& callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->profiles_changed_callbacks_.erase(
        std::remove_if(pimpl_->profiles_changed_callbacks_.begin(), 
                      pimpl_->profiles_changed_callbacks_.end(),
                      [&callback](const ProfilesChangedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->profiles_changed_callbacks_.end());
}

void ClientConnectionModel::remove_properties_changed_callback(const PropertiesChangedCallback& callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->properties_changed_callbacks_.erase(
        std::remove_if(pimpl_->properties_changed_callbacks_.begin(), 
                      pimpl_->properties_changed_callbacks_.end(),
                      [&callback](const PropertiesChangedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->properties_changed_callbacks_.end());
}

void ClientConnectionModel::remove_device_info_changed_callback(const DeviceInfoChangedCallback& callback) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    pimpl_->device_info_changed_callbacks_.erase(
        std::remove_if(pimpl_->device_info_changed_callbacks_.begin(), 
                      pimpl_->device_info_changed_callbacks_.end(),
                      [&callback](const DeviceInfoChangedCallback& cb) {
                          return cb.target<void()>() == callback.target<void()>();
                      }),
        pimpl_->device_info_changed_callbacks_.end());
}

void ClientConnectionModel::on_property_value_updated() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::cout << "Property value updated" << std::endl;
    
    for (const auto& callback : pimpl_->properties_changed_callbacks_) {
        callback();
    }
}

} // namespace ci_tool
