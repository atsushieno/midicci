#include "ClientConnectionModel.hpp"
#include "CIDeviceModel.hpp"
#include "midi-ci/properties/PropertyManager.hpp"
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
    std::vector<std::shared_ptr<MidiCIProfileState>> profiles_;
    std::vector<SubscriptionState> subscriptions_;
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

std::vector<std::shared_ptr<MidiCIProfileState>> ClientConnectionModel::get_profiles() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->profiles_;
}

void ClientConnectionModel::set_profile(uint8_t group, uint8_t address, const midi_ci::profiles::MidiCIProfileId& profile,
                                       bool new_enabled, uint16_t new_num_channels_requested) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->profiles_.begin(), pimpl_->profiles_.end(),
        [group, address, &profile](const std::shared_ptr<MidiCIProfileState>& state) {
            return state->get_group() == group && 
                   state->get_address() == address && 
                   state->get_profile() == profile;
        });
    
    if (it != pimpl_->profiles_.end()) {
        (*it)->set_enabled(new_enabled);
        (*it)->set_num_channels_requested(new_num_channels_requested);
    } else {
        auto new_profile = std::make_shared<MidiCIProfileState>(
            group, address, profile, new_enabled, new_num_channels_requested);
        pimpl_->profiles_.push_back(new_profile);
    }
    
    std::cout << "Set profile state - Group: " << static_cast<int>(group) 
              << ", Address: " << static_cast<int>(address) 
              << ", Enabled: " << new_enabled << std::endl;
}

std::vector<midi_ci::properties::PropertyMetadata> ClientConnectionModel::get_metadata_list() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return {};
}

std::vector<SubscriptionState> ClientConnectionModel::get_subscriptions() const {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    return pimpl_->subscriptions_;
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
    
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&resource](const SubscriptionState& sub) {
            return sub.property_id == resource;
        });
    
    if (it == pimpl_->subscriptions_.end()) {
        pimpl_->subscriptions_.emplace_back(resource, SubscriptionState::State::Subscribing);
    }
    
    std::cout << "Subscribing to property: " << resource << std::endl;
}

void ClientConnectionModel::unsubscribe_property(const std::string& resource) {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    
    auto it = std::find_if(pimpl_->subscriptions_.begin(), pimpl_->subscriptions_.end(),
        [&resource](const SubscriptionState& sub) {
            return sub.property_id == resource;
        });
    
    if (it != pimpl_->subscriptions_.end()) {
        it->state = SubscriptionState::State::Unsubscribed;
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
    
    auto& property_client = pimpl_->connection_->get_property_client_facade();
    
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
        pimpl_->profiles_.push_back(profile_state);
    }
    
    std::cout << "Updated profile list - count: " << pimpl_->profiles_.size() << std::endl;
}

void ClientConnectionModel::on_property_value_updated() {
    std::lock_guard<std::mutex> lock(pimpl_->mutex_);
    std::cout << "Property value updated" << std::endl;
}

} // namespace ci_tool
