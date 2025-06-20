#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CIToolRepositoryHandle* CIToolRepositoryHandle;

CIToolRepositoryHandle ci_tool_repository_create();
void ci_tool_repository_destroy(CIToolRepositoryHandle handle);

bool ci_tool_repository_initialize(CIToolRepositoryHandle handle);
void ci_tool_repository_shutdown(CIToolRepositoryHandle handle);

void ci_tool_repository_load_config(CIToolRepositoryHandle handle, const char* filename);
void ci_tool_repository_save_config(CIToolRepositoryHandle handle, const char* filename);
void ci_tool_repository_load_default_config(CIToolRepositoryHandle handle);
void ci_tool_repository_save_default_config(CIToolRepositoryHandle handle);

void ci_tool_repository_log(CIToolRepositoryHandle handle, const char* message, bool is_outgoing);
const char* ci_tool_repository_get_logs_json(CIToolRepositoryHandle handle);
void ci_tool_repository_clear_logs(CIToolRepositoryHandle handle);

uint32_t ci_tool_repository_get_muid(CIToolRepositoryHandle handle);

typedef struct CIDeviceManagerHandle* CIDeviceManagerHandle;

CIDeviceManagerHandle ci_tool_repository_get_device_manager(CIToolRepositoryHandle handle);
bool ci_device_manager_initialize(CIDeviceManagerHandle handle);
void ci_device_manager_shutdown(CIDeviceManagerHandle handle);

typedef struct CIDeviceModelHandle* CIDeviceModelHandle;

CIDeviceModelHandle ci_device_manager_get_device_model(CIDeviceManagerHandle handle);

void ci_device_model_send_discovery(CIDeviceModelHandle handle);
const char* ci_device_model_get_connections_json(CIDeviceModelHandle handle);
const char* ci_device_model_get_local_profiles_json(CIDeviceModelHandle handle);

bool ci_device_model_add_local_profile(CIDeviceModelHandle handle, const char* profile_json);
bool ci_device_model_remove_local_profile(CIDeviceModelHandle handle, uint8_t group, uint8_t address, const char* profile_id);
bool ci_device_model_update_local_profile(CIDeviceModelHandle handle, const char* profile_state_json);

bool ci_device_model_add_local_property(CIDeviceModelHandle handle, const char* property_json);
bool ci_device_model_remove_local_property(CIDeviceModelHandle handle, const char* property_id);
bool ci_device_model_update_property_value(CIDeviceModelHandle handle, const char* property_id, const char* res_id, const uint8_t* data, size_t data_length);

typedef struct MidiDeviceManagerHandle* MidiDeviceManagerHandle;

MidiDeviceManagerHandle ci_tool_repository_get_midi_device_manager(CIToolRepositoryHandle handle);
const char* midi_device_manager_get_input_devices_json(MidiDeviceManagerHandle handle);
const char* midi_device_manager_get_output_devices_json(MidiDeviceManagerHandle handle);
bool midi_device_manager_set_input_device(MidiDeviceManagerHandle handle, const char* device_id);
bool midi_device_manager_set_output_device(MidiDeviceManagerHandle handle, const char* device_id);

typedef void (*LogCallback)(const char* timestamp, bool is_outgoing, const char* message);
typedef void (*ConnectionsChangedCallback)();
typedef void (*ProfilesUpdatedCallback)();
typedef void (*PropertiesUpdatedCallback)();

void ci_tool_repository_set_log_callback(CIToolRepositoryHandle handle, LogCallback callback);
void ci_device_model_set_connections_changed_callback(CIDeviceModelHandle handle, ConnectionsChangedCallback callback);
void ci_device_model_set_profiles_updated_callback(CIDeviceModelHandle handle, ProfilesUpdatedCallback callback);
void ci_device_model_set_properties_updated_callback(CIDeviceModelHandle handle, PropertiesUpdatedCallback callback);

#ifdef __cplusplus
}
#endif
