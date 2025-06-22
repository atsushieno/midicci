#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CIToolRepositoryHandle* CIToolRepository;

CIToolRepository ci_tool_repository_create();
void ci_tool_repository_destroy(CIToolRepository handle);

bool ci_tool_repository_initialize(CIToolRepository handle);
void ci_tool_repository_shutdown(CIToolRepository handle);

void ci_tool_repository_load_config(CIToolRepository handle, const char* filename);
void ci_tool_repository_save_config(CIToolRepository handle, const char* filename);
void ci_tool_repository_load_default_config(CIToolRepository handle);
void ci_tool_repository_save_default_config(CIToolRepository handle);

void ci_tool_repository_log(CIToolRepository handle, const char* message, bool is_outgoing);
const char* ci_tool_repository_get_logs_json(CIToolRepository handle);
void ci_tool_repository_clear_logs(CIToolRepository handle);

// Test function to verify FFI is working
void ci_tool_test_ffi();

uint32_t ci_tool_repository_get_muid(CIToolRepository handle);

typedef struct CIDeviceManagerHandle* CIDeviceManager;

CIDeviceManager ci_tool_repository_get_device_manager(CIToolRepository handle);
bool ci_device_manager_initialize(CIDeviceManager handle);
void ci_device_manager_shutdown(CIDeviceManager handle);

typedef struct CIDeviceModelHandle* CIDeviceModel;

CIDeviceModel ci_device_manager_get_device_model(CIDeviceManager handle);

void ci_device_model_send_discovery(CIDeviceModel handle);
const char* ci_device_model_get_connections_json(CIDeviceModel handle);
const char* ci_device_model_get_local_profiles_json(CIDeviceModel handle);

bool ci_device_model_add_local_profile(CIDeviceModel handle, const char* profile_json);
bool ci_device_model_remove_local_profile(CIDeviceModel handle, uint8_t group, uint8_t address, const char* profile_id);
bool ci_device_model_update_local_profile(CIDeviceModel handle, const char* profile_state_json);

bool ci_device_model_add_local_property(CIDeviceModel handle, const char* property_json);
bool ci_device_model_remove_local_property(CIDeviceModel handle, const char* property_id);
bool ci_device_model_update_property_value(CIDeviceModel handle, const char* property_id, const char* res_id, const uint8_t* data, size_t data_length);

typedef struct MidiDeviceManagerHandle* MidiDeviceManager;

MidiDeviceManager ci_tool_repository_get_midi_device_manager(CIToolRepository handle);
const char* midi_device_manager_get_input_devices_json(MidiDeviceManager handle);
const char* midi_device_manager_get_output_devices_json(MidiDeviceManager handle);
bool midi_device_manager_set_input_device(MidiDeviceManager handle, const char* device_id);
bool midi_device_manager_set_output_device(MidiDeviceManager handle, const char* device_id);

typedef void (*LogCallback)(const char* timestamp, bool is_outgoing, const char* message);
typedef void (*ConnectionsChangedCallback)();
typedef void (*ProfilesUpdatedCallback)();
typedef void (*PropertiesUpdatedCallback)();

void ci_tool_repository_set_log_callback(CIToolRepository handle, LogCallback callback);
void ci_device_model_set_connections_changed_callback(CIDeviceModel handle, ConnectionsChangedCallback callback);
void ci_device_model_set_profiles_updated_callback(CIDeviceModel handle, ProfilesUpdatedCallback callback);
void ci_device_model_set_properties_updated_callback(CIDeviceModel handle, PropertiesUpdatedCallback callback);

#ifdef __cplusplus
}
#endif
