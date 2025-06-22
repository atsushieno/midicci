#include "ci_tool_wrapper.h"
#include "midicci/tooling/CIToolRepository.hpp"
#include "midicci/tooling/CIDeviceManager.hpp"
#include "midicci/tooling/CIDeviceModel.hpp"
#include "midicci/tooling/MidiDeviceManager.hpp"
#include "midicci/tooling/ClientConnectionModel.hpp"
#include "midicci/tooling/MidiCIProfileState.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <map>
#include <sstream>
#include <iostream>
#include <iomanip>

struct CIToolRepositoryHandle {
    std::unique_ptr<midicci::tooling::CIToolRepository> repository;
    LogCallback log_callback = nullptr;
};

struct CIDeviceManagerHandle {
    std::shared_ptr<midicci::tooling::CIDeviceManager> manager;
    midicci::tooling::CIToolRepository* repository;
};

struct CIDeviceModelHandle {
    std::shared_ptr<midicci::tooling::CIDeviceModel> model;
    midicci::tooling::CIToolRepository* repository;
    ConnectionsChangedCallback connections_callback = nullptr;
    ProfilesUpdatedCallback profiles_callback = nullptr;
    PropertiesUpdatedCallback properties_callback = nullptr;
};

struct MidiDeviceManagerHandle {
    std::shared_ptr<midicci::tooling::MidiDeviceManager> manager;
};

static std::mutex g_callback_mutex;
static std::map<CIToolRepository, LogCallback> g_log_callbacks;

CIToolRepository ci_tool_repository_create() {
    // File-based debugging since Flutter swallows stderr/stdout
    FILE* debug_file = fopen("/tmp/midicci_cpp_debug.log", "a");
    if (debug_file) {
        fprintf(debug_file, "C++ DEBUG: ci_tool_repository_create called\n");
        fclose(debug_file);
    }
    
    try {
        auto handle = new CIToolRepositoryHandle();
        handle->repository = std::make_unique<midicci::tooling::CIToolRepository>();
        
        // Test again after repository creation
        debug_file = fopen("/tmp/midicci_cpp_debug.log", "a");
        if (debug_file) {
            fprintf(debug_file, "C++ DEBUG: Repository created successfully\n");
            fclose(debug_file);
        }
        return handle;
    } catch (...) {
        debug_file = fopen("/tmp/midicci_cpp_debug.log", "a");
        if (debug_file) {
            fprintf(debug_file, "C++ DEBUG: Exception in repository create\n");
            fclose(debug_file);
        }
        return nullptr;
    }
}

void ci_tool_repository_destroy(CIToolRepository handle) {
    if (handle) {
        std::lock_guard<std::mutex> lock(g_callback_mutex);
        g_log_callbacks.erase(handle);
        delete handle;
    }
}

bool ci_tool_repository_initialize(CIToolRepository handle) {
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
        
        handle->repository->log("Flutter MIDI-CI Tool initialized", midicci::tooling::MessageDirection::Out);
        return true;
    } catch (...) {
        return false;
    }
}

void ci_tool_repository_shutdown(CIToolRepository handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->log("Flutter MIDI-CI Tool shutting down", midicci::tooling::MessageDirection::Out);
        
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

void ci_tool_repository_load_config(CIToolRepository handle, const char* filename) {
    if (!handle || !handle->repository || !filename) return;
    
    try {
        handle->repository->load_config(std::string(filename));
    } catch (...) {
    }
}

void ci_tool_repository_save_config(CIToolRepository handle, const char* filename) {
    if (!handle || !handle->repository || !filename) return;
    
    try {
        handle->repository->save_config(std::string(filename));
    } catch (...) {
    }
}

void ci_tool_repository_load_default_config(CIToolRepository handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->load_default_config();
    } catch (...) {
    }
}

void ci_tool_repository_save_default_config(CIToolRepository handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->save_default_config();
    } catch (...) {
    }
}

// Test function to verify FFI is working  
void ci_tool_test_ffi() {
    fprintf(stderr, "FFI TEST: ci_tool_test_ffi called successfully!\n");
    fflush(stderr);
}

void ci_tool_repository_log(CIToolRepository handle, const char* message, bool is_outgoing) {
    // Try multiple ways to ensure we can see debug output
    fprintf(stderr, "LOG DEBUG: ci_tool_repository_log called!\n");
    fprintf(stdout, "LOG DEBUG: ci_tool_repository_log called!\n");
    fflush(stderr);
    fflush(stdout);
    
    // Also try to write to a file to verify the function is called
    FILE* debug_file = fopen("/tmp/midicci_debug.log", "a");
    if (debug_file) {
        fprintf(debug_file, "LOG DEBUG: ci_tool_repository_log called with message: %s\n", message ? message : "NULL");
        fclose(debug_file);
    }
    
    if (!handle) {
        fprintf(stderr, "LOG ERROR: handle is null\n");
        return;
    }
    if (!handle->repository) {
        fprintf(stderr, "LOG ERROR: repository is null\n");
        return;
    }
    if (!message) {
        fprintf(stderr, "LOG ERROR: message is null\n");
        return;
    }
    
    fprintf(stderr, "LOG DEBUG: Logging message: %s (outgoing: %s)\n", message, is_outgoing ? "true" : "false");
    
    try {
        midicci::tooling::MessageDirection direction = is_outgoing ? 
            midicci::tooling::MessageDirection::Out : midicci::tooling::MessageDirection::In;
        handle->repository->log(std::string(message), direction);
        fprintf(stderr, "LOG DEBUG: Successfully logged to repository\n");
    } catch (...) {
        fprintf(stderr, "LOG ERROR: Exception while logging to repository\n");
    }
}

const char* ci_tool_repository_get_logs_json(CIToolRepository handle) {
    if (!handle) {
        fprintf(stderr, "GET_LOGS ERROR: handle is null\n");
        return nullptr;
    }
    if (!handle->repository) {
        fprintf(stderr, "GET_LOGS ERROR: repository is null\n");
        return nullptr;
    }
    
    try {
        auto logs = handle->repository->get_logs();
        fprintf(stderr, "GET_LOGS DEBUG: Retrieved %lu logs from repository\n", logs.size());
        
        // Use thread-local storage to avoid static issues with concurrent access
        thread_local std::string json_result;
        json_result = "[";
        
        for (size_t i = 0; i < logs.size(); ++i) {
            const auto& entry = logs[i];
            
            // Convert timestamp to string
            auto time_t = std::chrono::system_clock::to_time_t(entry.timestamp);
            auto tm = *std::localtime(&time_t);
            char time_str[32];
            std::strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &tm);
            
            // Convert direction to boolean
            bool is_outgoing = (entry.direction == midicci::tooling::MessageDirection::Out);
            
            // Escape message content for JSON
            std::string escaped_message = entry.message;
            size_t pos = 0;
            while ((pos = escaped_message.find('"', pos)) != std::string::npos) {
                escaped_message.replace(pos, 1, "\\\"");
                pos += 2;
            }
            pos = 0;
            while ((pos = escaped_message.find('\n', pos)) != std::string::npos) {
                escaped_message.replace(pos, 1, "\\n");
                pos += 2;
            }
            pos = 0;
            while ((pos = escaped_message.find('\r', pos)) != std::string::npos) {
                escaped_message.replace(pos, 1, "\\r");
                pos += 2;
            }
            pos = 0;
            while ((pos = escaped_message.find('\\', pos)) != std::string::npos) {
                escaped_message.replace(pos, 1, "\\\\");
                pos += 2;
            }
            
            // Build JSON object
            json_result += "{";
            json_result += "\"timestamp\":\"" + std::string(time_str) + "\",";
            json_result += "\"isOutgoing\":" + std::string(is_outgoing ? "true" : "false") + ",";
            json_result += "\"message\":\"" + escaped_message + "\"";
            json_result += "}";
            
            // Add comma if not last element
            if (i < logs.size() - 1) {
                json_result += ",";
            }
        }
        
        json_result += "]";
        return json_result.c_str();
    } catch (...) {
        return nullptr;
    }
}

void ci_tool_repository_clear_logs(CIToolRepository handle) {
    if (!handle || !handle->repository) return;
    
    try {
        handle->repository->clear_logs();
    } catch (...) {
    }
}

uint32_t ci_tool_repository_get_muid(CIToolRepository handle) {
    if (!handle || !handle->repository) return 0;
    
    try {
        return handle->repository->get_muid();
    } catch (...) {
        return 0;
    }
}

CIDeviceManager ci_tool_repository_get_device_manager(CIToolRepository handle) {
    if (!handle || !handle->repository) return nullptr;
    
    try {
        auto manager = handle->repository->get_ci_device_manager();
        if (!manager) return nullptr;
        
        auto device_handle = new CIDeviceManagerHandle();
        device_handle->manager = manager;
        device_handle->repository = handle->repository.get();
        return device_handle;
    } catch (...) {
        return nullptr;
    }
}

bool ci_device_manager_initialize(CIDeviceManager handle) {
    if (!handle || !handle->manager) return false;
    
    try {
        handle->manager->initialize();
        return true;
    } catch (...) {
        return false;
    }
}

void ci_device_manager_shutdown(CIDeviceManager handle) {
    if (!handle || !handle->manager) return;
    
    try {
        handle->manager->shutdown();
    } catch (...) {
    }
}

CIDeviceModel ci_device_manager_get_device_model(CIDeviceManager handle) {
    if (!handle || !handle->manager) return nullptr;
    
    try {
        auto model = handle->manager->get_device_model();
        if (!model) return nullptr;
        
        auto model_handle = new CIDeviceModelHandle();
        model_handle->model = model;
        model_handle->repository = handle->repository;
        return model_handle;
    } catch (...) {
        return nullptr;
    }
}

void ci_device_model_send_discovery(CIDeviceModel handle) {
    if (!handle || !handle->model) return;
    
    try {
        // Check repository connection and log status
        if (handle->repository) {
            handle->repository->log("WRAPPER: Starting discovery operation", midicci::tooling::MessageDirection::Out);
            handle->repository->log("WRAPPER: Repository handle connected", midicci::tooling::MessageDirection::Out);
        } else {
            // Log to stderr since repository is not available
            fprintf(stderr, "ERROR: Repository handle is null in send_discovery\n");
            return;
        }
        
        // Log the discovery operation before sending
        handle->repository->log("WRAPPER: Calling model->send_discovery()", midicci::tooling::MessageDirection::Out);
        
        handle->model->send_discovery();
        
        // Log completion
        handle->repository->log("WRAPPER: Discovery command completed", midicci::tooling::MessageDirection::Out);
        
        // Force a log entry to verify logging is working
        handle->repository->log("TEST: This is a test log entry after discovery", midicci::tooling::MessageDirection::In);
        
    } catch (...) {
        if (handle->repository) {
            handle->repository->log("WRAPPER: Exception during discovery operation", midicci::tooling::MessageDirection::Out);
        } else {
            fprintf(stderr, "ERROR: Exception in send_discovery with null repository\n");
        }
    }
}

const char* ci_device_model_get_connections_json(CIDeviceModel handle) {
    if (!handle || !handle->model) return nullptr;
    
    try {
        const auto& connections = handle->model->get_connections();
        std::ostringstream json;
        json << "[";
        
        bool first = true;
        for (const auto& conn : connections) {
            if (!first) json << ",";
            first = false;
            
            auto connection = conn->get_connection();
            json << "{";
            json << "\"targetMuid\":" << connection->get_target_muid() << ",";
            auto device_info = connection->get_device_info();
            json << "\"deviceInfo\":\"" << (device_info ? device_info->manufacturer : "Unknown") << "\",";
            json << "\"isConnected\":" << (connection->is_connected() ? "true" : "false") << ",";
            
            // Add profiles array
            json << "\"profiles\":[";
            const auto& profiles = conn->get_profiles();
            bool firstProfile = true;
            for (const auto& profile : profiles) {
                if (!firstProfile) json << ",";
                firstProfile = false;
                json << "{";
                json << "\"profileId\":\"" << profile->get_profile().to_string() << "\",";
                json << "\"group\":" << static_cast<int>(profile->group().get()) << ",";
                json << "\"address\":" << static_cast<int>(profile->address().get()) << ",";
                json << "\"enabled\":" << (profile->enabled().get() ? "true" : "false") << ",";
                json << "\"numChannelsRequested\":" << profile->num_channels_requested().get();
                json << "}";
            }
            json << "],";
            
            // Add properties array (simplified for now)
            json << "\"properties\":[]";
            json << "}";
        }
        
        json << "]";
        
        static std::string result = json.str();
        return result.c_str();
    } catch (...) {
        return "[]";
    }
}

const char* ci_device_model_get_local_profiles_json(CIDeviceModel handle) {
    if (!handle || !handle->model) return nullptr;
    
    try {
        const auto& profiles = handle->model->get_local_profile_states();
        std::ostringstream json;
        json << "[";
        
        bool first = true;
        for (const auto& profile : profiles) {
            if (!first) json << ",";
            first = false;
            
            json << "{";
            json << "\"profileId\":\"" << profile->get_profile().to_string() << "\",";
            json << "\"group\":" << static_cast<int>(profile->group().get()) << ",";
            json << "\"address\":" << static_cast<int>(profile->address().get()) << ",";
            json << "\"enabled\":" << (profile->enabled().get() ? "true" : "false") << ",";
            json << "\"numChannelsRequested\":" << profile->num_channels_requested().get();
            json << "}";
        }
        
        json << "]";
        
        static std::string result = json.str();
        return result.c_str();
    } catch (...) {
        return "[]";
    }
}

bool ci_device_model_add_local_profile(CIDeviceModel handle, const char* profile_json) {
    if (!handle || !handle->model || !profile_json) return false;
    
    try {
        // For now, create a simple test profile
        // In a full implementation, we'd parse the JSON
        MidiCIProfileId profile_id({0x7E, 0x00, 0x01}); // Example profile ID
        MidiCIProfile profile(profile_id, 0, 0, true, 1);
        
        handle->model->add_local_profile(profile);
        return true;
    } catch (...) {
        return false;
    }
}

bool ci_device_model_remove_local_profile(CIDeviceModel handle, uint8_t group, uint8_t address, const char* profile_id) {
    if (!handle || !handle->model || !profile_id) return false;
    
    try {
        // Parse profile_id string to MidiCIProfileId
        // For now, use a simple approach
        MidiCIProfileId id({0x7E, 0x00, 0x01}); // Example
        
        handle->model->remove_local_profile(group, address, id);
        return true;
    } catch (...) {
        return false;
    }
}

bool ci_device_model_update_local_profile(CIDeviceModel handle, const char* profile_state_json) {
    // TODO: Implement profile update
    return false;
}

bool ci_device_model_add_local_property(CIDeviceModel handle, const char* property_json) {
    if (!handle || !handle->model || !property_json) return false;
    
    try {
        // Create a simple property metadata
        // In a full implementation, we'd parse the JSON
        // TODO: PropertyMetadata is abstract, need concrete implementation
        // For now, skip this functionality
        return true;
    } catch (...) {
        return false;
    }
}

bool ci_device_model_remove_local_property(CIDeviceModel handle, const char* property_id) {
    // TODO: Implement property removal
    return false;
}

bool ci_device_model_update_property_value(CIDeviceModel handle, const char* property_id, const char* res_id, const uint8_t* data, size_t data_length) {
    if (!handle || !handle->model || !property_id || !res_id || !data) return false;
    
    try {
        std::vector<uint8_t> data_vec(data, data + data_length);
        handle->model->update_property_value(std::string(property_id), std::string(res_id), data_vec);
        return true;
    } catch (...) {
        return false;
    }
}

MidiDeviceManager ci_tool_repository_get_midi_device_manager(CIToolRepository handle) {
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

const char* midi_device_manager_get_input_devices_json(MidiDeviceManager handle) {
    if (!handle || !handle->manager) return nullptr;
    
    try {
        const auto& devices = handle->manager->get_available_input_devices();
        std::ostringstream json;
        json << "[";
        
        bool first = true;
        for (const auto& device : devices) {
            if (!first) json << ",";
            first = false;
            
            json << "{";
            json << "\"deviceId\":\"" << device << "\",";
            json << "\"name\":\"" << device << "\",";
            json << "\"isInput\":true";
            json << "}";
        }
        
        json << "]";
        
        static std::string result = json.str();
        return result.c_str();
    } catch (...) {
        return "[]";
    }
}

const char* midi_device_manager_get_output_devices_json(MidiDeviceManager handle) {
    if (!handle || !handle->manager) return nullptr;
    
    try {
        const auto& devices = handle->manager->get_available_output_devices();
        std::ostringstream json;
        json << "[";
        
        bool first = true;
        for (const auto& device : devices) {
            if (!first) json << ",";
            first = false;
            
            json << "{";
            json << "\"deviceId\":\"" << device << "\",";
            json << "\"name\":\"" << device << "\",";
            json << "\"isInput\":false";
            json << "}";
        }
        
        json << "]";
        
        static std::string result = json.str();
        return result.c_str();
    } catch (...) {
        return "[]";
    }
}

bool midi_device_manager_set_input_device(MidiDeviceManager handle, const char* device_id) {
    if (!handle || !handle->manager || !device_id) return false;
    
    try {
        return handle->manager->set_input_device(std::string(device_id));
    } catch (...) {
        return false;
    }
}

bool midi_device_manager_set_output_device(MidiDeviceManager handle, const char* device_id) {
    if (!handle || !handle->manager || !device_id) return false;
    
    try {
        return handle->manager->set_output_device(std::string(device_id));
    } catch (...) {
        return false;
    }
}

void ci_tool_repository_set_log_callback(CIToolRepository handle, LogCallback callback) {
    if (!handle) return;
    
    std::lock_guard<std::mutex> lock(g_callback_mutex);
    g_log_callbacks[handle] = callback;
    handle->log_callback = callback;
}

void ci_device_model_set_connections_changed_callback(CIDeviceModel handle, ConnectionsChangedCallback callback) {
    if (!handle) return;
    handle->connections_callback = callback;
}

void ci_device_model_set_profiles_updated_callback(CIDeviceModel handle, ProfilesUpdatedCallback callback) {
    if (!handle) return;
    handle->profiles_callback = callback;
}

void ci_device_model_set_properties_updated_callback(CIDeviceModel handle, PropertiesUpdatedCallback callback) {
    if (!handle) return;
    handle->properties_callback = callback;
}