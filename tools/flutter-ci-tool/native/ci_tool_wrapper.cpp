#include "ci_tool_wrapper.h"
#include "CIToolRepository.hpp"
#include "CIDeviceManager.hpp"
#include "CIDeviceModel.hpp"
#include "MidiDeviceManager.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <map>

using namespace ci_tool;

struct CIToolRepositoryHandle {
    std::unique_ptr<CIToolRepository> repository;
    LogCallback log_callback = nullptr;
};

struct CIDeviceManagerHandle {
    std::shared_ptr<CIDeviceManager> manager;
};

struct CIDeviceModelHandle {
    std::shared_ptr<CIDeviceModel> model;
    ConnectionsChangedCallback connections_callback = nullptr;
    ProfilesUpdatedCallback profiles_callback = nullptr;
    PropertiesUpdatedCallback properties_callback = nullptr;
};

struct MidiDeviceManagerHandle {
    std::shared_ptr<MidiDeviceManager> manager;
};

static std::mutex g_callback_mutex;
static std::map<CIToolRepositoryHandle, LogCallback> g_log_callbacks;

CIToolRepositoryHandle ci_tool_repository_create() {
    try {
        auto handle = new CIToolRepositoryHandle();
        handle->repository = std::make_unique<CIToolRepository>();
        return handle;
    } catch (...) {
        return nullptr;
    }
}

void ci_tool_repository_destroy(CIToolRepositoryHandle handle) {
    if (handle) {
        std::lock_guard<std::mutex> lock(g_callback_mutex);
        g_log_callbacks.erase(handle);
        delete handle;
    }
}

bool ci_tool_repository_initialize(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return false;
    
    try {
        auto midi_manager = handle->repository->get_midi_device_manager();
        auto ci_manager = handle->repository->get_ci_device_manager();
        
        if (midi_manager) {
            midi_manager->initialize();
        }
        if (ci_manager) {
            ci_manager->initialize();
        }
        
        handle->repository->log("Flutter MIDI-CI Tool initialized", MessageDirection::Out);
        return true;
    } catch (...) {
        return false;
    }
}

void ci_tool_repository_shutdown(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->log("Flutter MIDI-CI Tool shutting down", MessageDirection::Out);
        
        auto ci_manager = handle->repository->get_ci_device_manager();
        auto midi_manager = handle->repository->get_midi_device_manager();
        
        if (ci_manager) {
            ci_manager->shutdown();
        }
        if (midi_manager) {
            midi_manager->shutdown();
        }
    } catch (...) {
    }
}

void ci_tool_repository_load_config(CIToolRepositoryHandle handle, const char* filename) {
    if (!handle || !handle->repository || !filename) return;
    
    try {
        handle->repository->load_config(std::string(filename));
    } catch (...) {
    }
}

void ci_tool_repository_save_config(CIToolRepositoryHandle handle, const char* filename) {
    if (!handle || !handle->repository || !filename) return;
    
    try {
        handle->repository->save_config(std::string(filename));
    } catch (...) {
    }
}

void ci_tool_repository_load_default_config(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->load_default_config();
    } catch (...) {
    }
}

void ci_tool_repository_save_default_config(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->save_default_config();
    } catch (...) {
    }
}

void ci_tool_repository_log(CIToolRepositoryHandle handle, const char* message, bool is_outgoing) {
    if (!handle || !handle->repository || !message) return;
    
    try {
        MessageDirection direction = is_outgoing ? MessageDirection::Out : MessageDirection::In;
        handle->repository->log(std::string(message), direction);
    } catch (...) {
    }
}

const char* ci_tool_repository_get_logs_json(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return nullptr;
    
    try {
        static std::string json_logs = "[]";
        return json_logs.c_str();
    } catch (...) {
        return nullptr;
    }
}

void ci_tool_repository_clear_logs(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->clear_logs();
    } catch (...) {
    }
}

uint32_t ci_tool_repository_get_muid(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return 0;
    
    try {
        return handle->repository->get_muid();
    } catch (...) {
        return 0;
    }
}

CIDeviceManagerHandle ci_tool_repository_get_device_manager(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return nullptr;
    
    try {
        auto manager = handle->repository->get_ci_device_manager();
        if (!manager) return nullptr;
        
        auto device_handle = new CIDeviceManagerHandle();
        device_handle->manager = manager;
        return device_handle;
    } catch (...) {
        return nullptr;
    }
}

bool ci_device_manager_initialize(CIDeviceManagerHandle handle) {
    if (!handle || !handle->manager) return false;
    
    try {
        handle->manager->initialize();
        return true;
    } catch (...) {
        return false;
    }
}

void ci_device_manager_shutdown(CIDeviceManagerHandle handle) {
    if (!handle || !handle->manager) return;
    
    try {
        handle->manager->shutdown();
    } catch (...) {
    }
}

CIDeviceModelHandle ci_device_manager_get_device_model(CIDeviceManagerHandle handle) {
    if (!handle || !handle->manager) return nullptr;
    
    try {
        auto model = handle->manager->get_device_model();
        if (!model) return nullptr;
        
        auto model_handle = new CIDeviceModelHandle();
        model_handle->model = model;
        return model_handle;
    } catch (...) {
        return nullptr;
    }
}

void ci_device_model_send_discovery(CIDeviceModelHandle handle) {
    if (!handle || !handle->model) return;
    
    try {
        handle->model->send_discovery();
    } catch (...) {
    }
}

const char* ci_device_model_get_connections_json(CIDeviceModelHandle handle) {
    if (!handle || !handle->model) return nullptr;
    
    try {
        static std::string json_connections = "[]";
        return json_connections.c_str();
    } catch (...) {
        return nullptr;
    }
}

const char* ci_device_model_get_local_profiles_json(CIDeviceModelHandle handle) {
    if (!handle || !handle->model) return nullptr;
    
    try {
        static std::string json_profiles = "[]";
        return json_profiles.c_str();
    } catch (...) {
        return nullptr;
    }
}

bool ci_device_model_add_local_profile(CIDeviceModelHandle handle, const char* profile_json) {
    return false;
}

bool ci_device_model_remove_local_profile(CIDeviceModelHandle handle, uint8_t group, uint8_t address, const char* profile_id) {
    return false;
}

bool ci_device_model_update_local_profile(CIDeviceModelHandle handle, const char* profile_state_json) {
    return false;
}

bool ci_device_model_add_local_property(CIDeviceModelHandle handle, const char* property_json) {
    return false;
}

bool ci_device_model_remove_local_property(CIDeviceModelHandle handle, const char* property_id) {
    return false;
}

bool ci_device_model_update_property_value(CIDeviceModelHandle handle, const char* property_id, const char* res_id, const uint8_t* data, size_t data_length) {
    return false;
}

MidiDeviceManagerHandle ci_tool_repository_get_midi_device_manager(CIToolRepositoryHandle handle) {
    if (!handle || !handle->repository) return nullptr;
    
    try {
        auto manager = handle->repository->get_midi_device_manager();
        if (!manager) return nullptr;
        
        auto midi_handle = new MidiDeviceManagerHandle();
        midi_handle->manager = manager;
        return midi_handle;
    } catch (...) {
        return nullptr;
    }
}

const char* midi_device_manager_get_input_devices_json(MidiDeviceManagerHandle handle) {
    static std::string json_devices = "[]";
    return json_devices.c_str();
}

const char* midi_device_manager_get_output_devices_json(MidiDeviceManagerHandle handle) {
    static std::string json_devices = "[]";
    return json_devices.c_str();
}

bool midi_device_manager_set_input_device(MidiDeviceManagerHandle handle, const char* device_id) {
    return false;
}

bool midi_device_manager_set_output_device(MidiDeviceManagerHandle handle, const char* device_id) {
    return false;
}

void ci_tool_repository_set_log_callback(CIToolRepositoryHandle handle, LogCallback callback) {
    if (!handle) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    g_log_callbacks[handle] = callback;
    handle->log_callback = callback;
}

void ci_device_model_set_connections_changed_callback(CIDeviceModelHandle handle, ConnectionsChangedCallback callback) {
    if (!handle) return;
    handle->connections_callback = callback;
}

void ci_device_model_set_profiles_updated_callback(CIDeviceModelHandle handle, ProfilesUpdatedCallback callback) {
    if (!handle) return;
    handle->profiles_callback = callback;
}

void ci_device_model_set_properties_updated_callback(CIDeviceModelHandle handle, PropertiesUpdatedCallback callback) {
    if (!handle) return;
    handle->properties_callback = callback;
}
