#include "ci_tool_repository_wrapper.h"
#include "ci_device_manager_wrapper.h"
#include "midi_device_manager_wrapper.h"

Napi::FunctionReference CIToolRepositoryWrapper::constructor;

Napi::Object CIToolRepositoryWrapper::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "CIToolRepository", {
    InstanceMethod("initialize", &CIToolRepositoryWrapper::Initialize),
    InstanceMethod("shutdown", &CIToolRepositoryWrapper::Shutdown),
    InstanceMethod("sendDiscovery", &CIToolRepositoryWrapper::SendDiscovery),
    InstanceMethod("getLogs", &CIToolRepositoryWrapper::GetLogs),
    InstanceMethod("clearLogs", &CIToolRepositoryWrapper::ClearLogs),
    InstanceMethod("getMUID", &CIToolRepositoryWrapper::GetMUID),
    InstanceMethod("getCIDeviceManager", &CIToolRepositoryWrapper::GetCIDeviceManager),
    InstanceMethod("getMidiDeviceManager", &CIToolRepositoryWrapper::GetMidiDeviceManager),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("CIToolRepository", func);
  return exports;
}

CIToolRepositoryWrapper::CIToolRepositoryWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<CIToolRepositoryWrapper>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  
  repository_ = std::make_unique<ci_tool::CIToolRepository>();
}

Napi::Value CIToolRepositoryWrapper::Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    auto ci_manager = repository_->get_ci_device_manager();
    if (ci_manager) {
      ci_manager->initialize();
    }
    repository_->log("MIDI-CI Repository initialized", ci_tool::MessageDirection::Out);
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to initialize repository: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::Shutdown(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    return Napi::Boolean::New(env, true);
  }
  
  try {
    auto ci_manager = repository_->get_ci_device_manager();
    if (ci_manager) {
      ci_manager->shutdown();
    }
    repository_->log("MIDI-CI Repository shutdown", ci_tool::MessageDirection::Out);
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to shutdown repository: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::SendDiscovery(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    auto ci_manager = repository_->get_ci_device_manager();
    if (ci_manager) {
      auto device_model = ci_manager->get_device_model();
      if (device_model) {
        device_model->send_discovery();
        repository_->log("Discovery inquiry sent", ci_tool::MessageDirection::Out);
        return Napi::Boolean::New(env, true);
      } else {
        Napi::Error::New(env, "CI Device Model not available").ThrowAsJavaScriptException();
        return env.Null();
      }
    } else {
      Napi::Error::New(env, "CI Device Manager not available").ThrowAsJavaScriptException();
      return env.Null();
    }
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to send discovery: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::GetLogs(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    auto logs = repository_->get_logs();
    Napi::Array result = Napi::Array::New(env, logs.size());
    
    for (size_t i = 0; i < logs.size(); i++) {
      Napi::Object logEntry = Napi::Object::New(env);
      logEntry.Set("message", logs[i].message);
      logEntry.Set("direction", logs[i].direction == ci_tool::MessageDirection::In ? "In" : "Out");
      logEntry.Set("timestamp", Napi::Date::New(env, std::chrono::duration_cast<std::chrono::milliseconds>(logs[i].timestamp.time_since_epoch()).count()));
      result[i] = logEntry;
    }
    
    return result;
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get logs: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::ClearLogs(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    repository_->clear_logs();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to clear logs: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::GetMUID(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    return Napi::Number::New(env, repository_->get_muid());
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get MUID: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::GetCIDeviceManager(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    auto ci_manager = repository_->get_ci_device_manager();
    if (ci_manager) {
      Napi::Object result = CIDeviceManagerWrapper::constructor.New({});
      CIDeviceManagerWrapper* wrapper = CIDeviceManagerWrapper::Unwrap(result);
      wrapper->SetManager(ci_manager);
      return result;
    } else {
      return env.Null();
    }
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get CI device manager: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIToolRepositoryWrapper::GetMidiDeviceManager(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    auto midi_manager = repository_->get_midi_device_manager();
    if (midi_manager) {
      Napi::Object result = MidiDeviceManagerWrapper::constructor.New({});
      MidiDeviceManagerWrapper* wrapper = MidiDeviceManagerWrapper::Unwrap(result);
      wrapper->SetManager(midi_manager);
      return result;
    } else {
      return env.Null();
    }
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get MIDI device manager: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}
