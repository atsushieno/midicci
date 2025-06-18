#include "midi-ci/core/ClientConnection.hpp"
#include "midi-ci/messages/Message.hpp"
#include "midi-ci/core/MidiCIConstants.hpp"
#include "midi-ci/core/CIFactory.hpp"
#include "midi-ci/core/MidiCIDevice.hpp"
#include "midi-ci/profiles/ProfileClientFacade.hpp"
#include "midi-ci/properties/PropertyClientFacade.hpp"
#include "midi-ci/json/Json.hpp"
#include <mutex>

namespace midi_ci {
namespace core {

class ClientConnection::Impl {
public:
    explicit Impl(uint32_t target_muid, MidiCIDevice& device, ClientConnection& conn)
        : target_muid_(target_muid), connected_(true),
          profile_client_facade_(std::make_unique<profiles::ProfileClientFacade>(device, conn)),
          property_client_facade_(std::make_unique<properties::PropertyClientFacade>(device, conn)),
          device_info_(nullptr), channel_list_(nullptr), json_schema_(nullptr) {}
    
    uint32_t target_muid_;
    bool connected_;
    MessageCallback message_callback_;
    CIOutputSender ci_output_sender_;
    std::unique_ptr<profiles::ProfileClientFacade> profile_client_facade_;
    std::unique_ptr<properties::PropertyClientFacade> property_client_facade_;
    std::unique_ptr<messages::DeviceInfo> device_info_;
    std::unique_ptr<json::JsonValue> channel_list_;
    std::unique_ptr<json::JsonValue> json_schema_;
    mutable std::recursive_mutex mutex_;
};

ClientConnection::ClientConnection(MidiCIDevice& device, uint32_t target_muid)
    : pimpl_(std::make_unique<Impl>(target_muid, device, *this)) {}

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

void ClientConnection::send_message(const midi_ci::messages::Message& message) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    if (pimpl_->connected_ && pimpl_->ci_output_sender_) {
        auto packets = message.serialize_multi();
        for (const auto& packet : packets) {
            std::vector<uint8_t> sysex_data;
            sysex_data.push_back(0xF0);
            sysex_data.insert(sysex_data.end(), packet.begin(), packet.end());
            sysex_data.push_back(0xF7);
            
            pimpl_->ci_output_sender_(0, sysex_data);
        }
    }
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
                midi_ci::messages::MessageType msg_type = static_cast<midi_ci::messages::MessageType>(midi_ci_data[4]);
                
                switch (msg_type) {
                    case midi_ci::messages::MessageType::GetPropertyData: {
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
                                
                                midi_ci::messages::Common common(source_muid, dest_muid, 0x7F, 0);
                                midi_ci::messages::GetPropertyData msg(common, request_id, header);
                                pimpl_->message_callback_(msg);
                            }
                        }
                        break;
                    }
                    case midi_ci::messages::MessageType::SetPropertyData: {
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
                                    
                                    midi_ci::messages::Common common(source_muid, dest_muid, 0x7F, 0);
                                    midi_ci::messages::SetPropertyData msg(common, request_id, header, body);
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

profiles::ProfileClientFacade& ClientConnection::get_profile_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

const profiles::ProfileClientFacade& ClientConnection::get_profile_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->profile_client_facade_;
}

properties::PropertyClientFacade& ClientConnection::get_property_client_facade() {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

const properties::PropertyClientFacade& ClientConnection::get_property_client_facade() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return *pimpl_->property_client_facade_;
}

void ClientConnection::set_device_info(const messages::DeviceInfo& device_info) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->device_info_ = std::make_unique<messages::DeviceInfo>(device_info);
}

const messages::DeviceInfo* ClientConnection::get_device_info() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->device_info_.get();
}

void ClientConnection::set_channel_list(const json::JsonValue& channel_list) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->channel_list_ = std::make_unique<json::JsonValue>(channel_list);
}

const json::JsonValue* ClientConnection::get_channel_list() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->channel_list_.get();
}

void ClientConnection::set_json_schema(const json::JsonValue& json_schema) {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    pimpl_->json_schema_ = std::make_unique<json::JsonValue>(json_schema);
}

const json::JsonValue* ClientConnection::get_json_schema() const {
    std::lock_guard<std::recursive_mutex> lock(pimpl_->mutex_);
    return pimpl_->json_schema_.get();
}

} // namespace core
} // namespace midi_ci
