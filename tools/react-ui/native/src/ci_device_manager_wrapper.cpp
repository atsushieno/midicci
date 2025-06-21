#include "ci_device_manager_wrapper.h"
#include "ci_tool_repository_wrapper.h"

Napi::FunctionReference CIDeviceManagerWrapper::constructor;

Napi::Object CIDeviceManagerWrapper::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "CIDeviceManager", {
    InstanceMethod("getConnections", &CIDeviceManagerWrapper::GetConnections),
    InstanceMethod("setProfile", &CIDeviceManagerWrapper::SetProfile),
    InstanceMethod("subscribeProperty", &CIDeviceManagerWrapper::SubscribeProperty),
    InstanceMethod("unsubscribeProperty", &CIDeviceManagerWrapper::UnsubscribeProperty),
    InstanceMethod("refreshPropertyValue", &CIDeviceManagerWrapper::RefreshPropertyValue),
    InstanceMethod("createProperty", &CIDeviceManagerWrapper::CreateProperty),
    InstanceMethod("updatePropertyMetadata", &CIDeviceManagerWrapper::UpdatePropertyMetadata),
    InstanceMethod("updatePropertyValue", &CIDeviceManagerWrapper::UpdatePropertyValue),
    InstanceMethod("removeProperty", &CIDeviceManagerWrapper::RemoveProperty),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("CIDeviceManager", func);
  return exports;
}

CIDeviceManagerWrapper::CIDeviceManagerWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<CIDeviceManagerWrapper>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
  
  if (info.Length() > 0 && info[0].IsObject()) {
    auto repo_wrapper = Napi::ObjectWrap<CIToolRepositoryWrapper>::Unwrap(info[0].As<Napi::Object>());
    if (repo_wrapper && repo_wrapper->GetRepository()) {
      manager_ = repo_wrapper->GetRepository()->get_ci_device_manager();
    }
  }
}

Napi::Value CIDeviceManagerWrapper::GetConnections(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    return Napi::Array::New(env, 0);
  }
  
  try {
    auto device_model = manager_->get_device_model();
    if (!device_model) {
      return Napi::Array::New(env, 0);
    }
    
    const auto& connections = device_model->get_connections();
    auto connections_vec = connections.to_vector();
    Napi::Array result = Napi::Array::New(env, connections_vec.size());
    
    for (size_t i = 0; i < connections_vec.size(); i++) {
      auto& conn = connections_vec[i];
      if (!conn) continue;
      
      Napi::Object connectionObj = Napi::Object::New(env);
      
      Napi::Object connInfo = Napi::Object::New(env);
      if (conn->get_connection()) {
        connInfo.Set("targetMUID", conn->get_connection()->get_target_muid());
        connInfo.Set("productInstanceId", "");
        connInfo.Set("maxSimultaneousPropertyRequests", 1);
      }
      connectionObj.Set("connection", connInfo);
      
      connectionObj.Set("profiles", Napi::Array::New(env, 0));
      connectionObj.Set("subscriptions", Napi::Array::New(env, 0));
      connectionObj.Set("properties", Napi::Array::New(env, 0));
      
      Napi::Object deviceInfo = Napi::Object::New(env);
      deviceInfo.Set("manufacturer", "");
      deviceInfo.Set("manufacturerId", 0);
      deviceInfo.Set("family", "");
      deviceInfo.Set("familyId", 0);
      deviceInfo.Set("model", "");
      deviceInfo.Set("modelId", 0);
      deviceInfo.Set("version", "");
      deviceInfo.Set("versionId", 0);
      connectionObj.Set("deviceInfo", deviceInfo);
      
      result[i] = connectionObj;
    }
    
    return result;
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get connections: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIDeviceManagerWrapper::SetProfile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "Device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 5) {
    Napi::TypeError::New(env, "Expected 5 arguments: group, address, profile, enabled, numChannels").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    uint8_t group = info[0].As<Napi::Number>().Uint32Value();
    uint8_t address = info[1].As<Napi::Number>().Uint32Value();
    
    Napi::Object profileObj = info[2].As<Napi::Object>();
    Napi::Array bytesArray = profileObj.Get("bytes").As<Napi::Array>();
    std::vector<uint8_t> profileBytes;
    for (uint32_t i = 0; i < bytesArray.Length(); i++) {
      profileBytes.push_back(bytesArray.Get(i).As<Napi::Number>().Uint32Value());
    }
    
    bool enabled = info[3].As<Napi::Boolean>().Value();
    uint16_t numChannels = info[4].As<Napi::Number>().Uint32Value();
    
    auto device_model = manager_->get_device_model();
    if (device_model) {
      midicci::profilecommonrules::MidiCIProfileId profile_id{profileBytes};
      midicci::profilecommonrules::MidiCIProfile profile{profile_id, group, address, enabled, numChannels};
      device_model->add_local_profile(profile);
    }
    
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to set profile: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIDeviceManagerWrapper::SubscribeProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "Device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected property ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    std::string propertyId = info[0].As<Napi::String>().Utf8Value();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to subscribe property: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIDeviceManagerWrapper::UnsubscribeProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "Device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected property ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    std::string propertyId = info[0].As<Napi::String>().Utf8Value();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to unsubscribe property: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIDeviceManagerWrapper::RefreshPropertyValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "Device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Expected property ID").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    std::string propertyId = info[0].As<Napi::String>().Utf8Value();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to refresh property value: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value CIDeviceManagerWrapper::CreateProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Value CIDeviceManagerWrapper::UpdatePropertyMetadata(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Value CIDeviceManagerWrapper::UpdatePropertyValue(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Value CIDeviceManagerWrapper::RemoveProperty(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}
