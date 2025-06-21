#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "MidiCIConstants.hpp"
#include "MidiCIChannelList.hpp"
#include "MidiCIProfile.hpp"
#include "ObservablePropertyList.hpp"

namespace midicci {

struct MidiCIDeviceConfiguration {
    // Device identification
    DeviceInfo device_info{0x654321, 0x4321, 0x765, 0x00000002,
                           "atsushieno", "cpp-midi-ci", "cpp-midi-ci-tool", "0.1", "ABCDEFGH"};
    
    // Channel configuration
    MidiCIChannelList channel_list;
    
    // JSON schema
    std::string json_schema_string;
    
    // Basic capabilities
    uint8_t capability_inquiry_supported = static_cast<uint8_t>(MidiCISupportedCategories::THREE_P);
    int receivable_max_sysex_size = DEFAULT_RECEIVABLE_MAX_SYSEX_SIZE;
    uint8_t max_simultaneous_property_requests = DEFAULT_MAX_SIMULTANEOUS_PROPERTY_REQUESTS;
    int max_property_chunk_size = DEFAULT_MAX_PROPERTY_CHUNK_SIZE;
    
    // Group and addressing
    uint8_t group = 0;
    uint8_t output_path_id = 0;
    uint8_t function_block = NO_FUNCTION_BLOCK;
    std::string product_instance_id = "cpp-midi-ci";
    
    // Auto-send discovery flags
    bool auto_send_endpoint_inquiry = true;
    bool auto_send_profile_inquiry = true;
    bool auto_send_property_exchange_capabilities_inquiry = true;
    bool auto_send_process_inquiry = true;
    bool auto_send_get_resource_list = true;
    bool auto_send_get_device_info = true;
    
    // Profile configuration
    std::vector<MidiCIProfile> local_profiles;
    
    // Process inquiry configuration
    uint8_t process_inquiry_supported_features = static_cast<uint8_t>(MidiCIProcessInquiryFeatures::MIDI_MESSAGE_REPORT);
    uint8_t midi_message_report_message_data_control = static_cast<uint8_t>(MidiMessageReportDataControl::Full);
    uint8_t midi_message_report_system_messages = static_cast<uint8_t>(MidiMessageReportSystemMessagesFlags::All);
    uint8_t midi_message_report_channel_controller_messages = static_cast<uint8_t>(MidiMessageReportChannelControllerFlags::All);
    uint8_t midi_message_report_note_data_messages = static_cast<uint8_t>(MidiMessageReportNoteDataFlags::All);
    
    // Property exchange
    std::vector<PropertyValue> property_values;
    std::vector<std::unique_ptr<PropertyMetadata>> property_metadata_list;
    
    // Constructor with backward compatibility for existing usage
    MidiCIDeviceConfiguration(int max_sysex = DEFAULT_RECEIVABLE_MAX_SYSEX_SIZE,
                             int max_chunk = DEFAULT_MAX_PROPERTY_CHUNK_SIZE,
                             const std::string& prod_id = "cpp-midi-ci", 
                             uint8_t group = 0)
        : receivable_max_sysex_size(max_sysex), max_property_chunk_size(max_chunk),
          group(group), product_instance_id(prod_id) {}
};

} // namespace
