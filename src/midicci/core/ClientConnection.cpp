#include "midicci/core/ClientConnection.hpp"
#include "midicci/messages/Message.hpp"
#include "midicci/core/MidiCIConstants.hpp"
#include "midicci/core/CIFactory.hpp"
#include "midicci/core/MidiCIDevice.hpp"
#include "midicci/profiles/ProfileClientFacade.hpp"
#include "midicci/properties/PropertyClientFacade.hpp"
#include "midicci/json_ish/Json.hpp"
#include <mutex>

namespace midicci {
namespace core {

class ClientConnection::Impl {
public:
    explicit Impl(uint32_t target_muid, MidiCIDevice& device, DeviceDetails& device_details, ClientConnection& conn)
        : target_muid_(target_muid), connected_(true),
          profile_client_facade_(std::make_unique<profilecommonrules::ProfileClientFacade>(device, conn)),
          property_client_facade_(std::make_unique<propertycommonrules::PropertyClientFacade>(device, conn)),
          channel_list_(nullptr), json_schema_(nullptr) {
        // those string fields are unknown at DiscoveryReply.
        device_info_ = std::make_unique<DeviceInfo>(device_details.manufacturer,
                                                    device_details.family,
                                                    device_details.modelNumber,
                                                    device_details.softwareRevisionLevel,
                                                    "", "", "", "", "");
    }
    
    uint32_t target_muid_;
    bool connected_;
    MessageCallback message_callback_;
    CIOutputSender ci_output_sender_;
    std::unique_ptr<profilecommonrules::ProfileClientFacade> profile_client_facade_;
    std::unique_ptr<propertycommonrules::PropertyClientFacade> property_client_facade_;
    std::unique_ptr<DeviceInfo> device_info_;
    std::unique_ptr<JsonValue> channel_list_;
    std::unique_ptr<JsonValue> json_schema_;
    mutable std::recursive_mutex mutex_;
};

ClientConnection::ClientConnection(MidiCIDevice& device, uint32_t target_muid, DeviceDetails device_details)
    : pimpl_(std::make_unique<Impl>(target_muid, device, device_details, *this)) {}

ClientConnection::~ClientConnection() = default;

uint32_t ClientConnection::get_target_muid() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->target_muid_;
}

void ClientConnection::set_message_callback(MessageCallback callback) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->message_callback_ = std::move(callback);
}

void ClientConnection::set_ci_output_sender(CIOutputSender sender) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->ci_output_sender_ = std::move(sender);
}



void ClientConnection::process_incoming_sysex(uint8_t group, const std::vector<uint8_t>& sysex_data) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->message_callback_ && sysex_data.size() >= 5) {
        if (sysex_data[0] == 0xF0 && 
            sysex_data[1] == 0x7E &&
            sysex_data[3] == 0x0D &&
            sysex_data.back() == 0xF7) {
            
            std::vector<uint8_t> midi_ci_data(sysex_data.begin() + 1, sysex_data.end() - 1);
            
            if (midi_ci_data.size() >= 6) {
                midicci::MessageType msg_type = static_cast<midicci::MessageType>(midi_ci_data[4]);
                
                switch (msg_type) {
                    case midicci::MessageType::GetPropertyData: {
                        if (midi_ci_data.size() >= 21) {
                            uint32_t source_muid = (midi_ci_data[5]) | (midi_ci_data[6] << 7) | (midi_ci_data[7] << 14) | (midi_ci_data[8] << 21);
                            uint32_t dest_muid = (midi_ci_data[9]) | (midi_ci_data[10] << 7) | (midi_ci_data[11] << 14) | (midi_ci_data[12] << 21);
                            uint8_t request_id = midi_ci_data[13];
                            uint16_t header_size = midi_ci_data[14] | (midi_ci_data[15] << 7);
                            
                            if (16 + header_size + 6 <= midi_ci_data.size()) {
                                std::vector<uint8_t> header(midi_ci_data.begin() + 16, midi_ci_data.begin() + 16 + header_size);
                                size_t chunk_info_offset = 16 + header_size;
                                uint16_t num_chunks = midi_ci_data[chunk_info_offset] | (midi_ci_data[chunk_info_offset + 1] << 7);
                                uint16_t chunk_index = midi_ci_data[chunk_info_offset + 2] | (midi_ci_data[chunk_info_offset + 3] << 7);
                                uint16_t chunk_data_size = midi_ci_data[chunk_info_offset + 4] | (midi_ci_data[chunk_info_offset + 5] << 7);
                                
                                midicci::Common common(source_muid, dest_muid, 0x7F, 0);
                                midicci::GetPropertyData msg(common, request_id, header);
                                pimpl_->message_callback_(msg);
                            }
                        }
                        break;
                    }
                    case midicci::MessageType::SetPropertyData: {
                        if (midi_ci_data.size() >= 21) {
                            uint32_t source_muid = (midi_ci_data[5]) | (midi_ci_data[6] << 7) | (midi_ci_data[7] << 14) | (midi_ci_data[8] << 21);
                            uint32_t dest_muid = (midi_ci_data[9]) | (midi_ci_data[10] << 7) | (midi_ci_data[11] << 14) | (midi_ci_data[12] << 21);
                            uint8_t request_id = midi_ci_data[13];
                            uint16_t header_size = midi_ci_data[14] | (midi_ci_data[15] << 7);
                            
                            if (16 + header_size + 6 <= midi_ci_data.size()) {
                                std::vector<uint8_t> header(midi_ci_data.begin() + 16, midi_ci_data.begin() + 16 + header_size);
                                size_t chunk_info_offset = 16 + header_size;
                                uint16_t num_chunks = midi_ci_data[chunk_info_offset] | (midi_ci_data[chunk_info_offset + 1] << 7);
                                uint16_t chunk_index = midi_ci_data[chunk_info_offset + 2] | (midi_ci_data[chunk_info_offset + 3] << 7);
                                uint16_t chunk_data_size = midi_ci_data[chunk_info_offset + 4] | (midi_ci_data[chunk_info_offset + 5] << 7);
                                
                                if (chunk_info_offset + 6 + chunk_data_size <= midi_ci_data.size()) {
                                    std::vector<uint8_t> body(midi_ci_data.begin() + chunk_info_offset + 6, 
                                                            midi_ci_data.begin() + chunk_info_offset + 6 + chunk_data_size);
                                    
                                    midicci::Common common(source_muid, dest_muid, 0x7F, 0);
                                    midicci::SetPropertyData msg(common, request_id, header, body);
                                    pimpl_->message_callback_(msg);
                                }
                            }
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }
    }
}

bool ClientConnection::is_connected() const noexcept {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->connected_;
}

void ClientConnection::disconnect() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->connected_ = false;
}

profilecommonrules::ProfileClientFacade& ClientConnection::get_profile_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

const profilecommonrules::ProfileClientFacade& ClientConnection::get_profile_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

propertycommonrules::PropertyClientFacade& ClientConnection::get_property_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

const propertycommonrules::PropertyClientFacade& ClientConnection::get_property_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

void ClientConnection::set_device_info(const DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->device_info_ = std::make_unique<DeviceInfo>(device_info);
}

const DeviceInfo* ClientConnection::get_device_info() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->device_info_.get();
}

void ClientConnection::set_channel_list(const JsonValue& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->channel_list_ = std::make_unique<JsonValue>(channel_list);
}

const JsonValue* ClientConnection::get_channel_list() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->channel_list_.get();
}

void ClientConnection::set_json_schema(const JsonValue& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->json_schema_ = std::make_unique<JsonValue>(json_schema);
}

const JsonValue* ClientConnection::get_json_schema() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->json_schema_.get();
}

} // namespace core
} // namespace midi_ci
