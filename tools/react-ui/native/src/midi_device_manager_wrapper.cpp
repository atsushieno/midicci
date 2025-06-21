#include "midi_device_manager_wrapper.h"

Napi::FunctionReference MidiDeviceManagerWrapper::constructor;

Napi::Object MidiDeviceManagerWrapper::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "MidiDeviceManager", {
    InstanceMethod("initialize", &MidiDeviceManagerWrapper::Initialize),
    InstanceMethod("shutdown", &MidiDeviceManagerWrapper::Shutdown),
    InstanceMethod("getDevices", &MidiDeviceManagerWrapper::GetDevices),
    InstanceMethod("openDevice", &MidiDeviceManagerWrapper::OpenDevice),
    InstanceMethod("closeDevice", &MidiDeviceManagerWrapper::CloseDevice),
  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("MidiDeviceManager", func);
  return exports;
}

MidiDeviceManagerWrapper::MidiDeviceManagerWrapper(const Napi::CallbackInfo& info)
    : Napi::ObjectWrap<MidiDeviceManagerWrapper>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);
}

Napi::Value MidiDeviceManagerWrapper::Initialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "MIDI device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    manager_->initialize();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to initialize MIDI device manager: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value MidiDeviceManagerWrapper::Shutdown(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    return Napi::Boolean::New(env, true);
  }
  
  try {
    manager_->shutdown();
    return Napi::Boolean::New(env, true);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to shutdown MIDI device manager: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value MidiDeviceManagerWrapper::GetDevices(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    return Napi::Array::New(env, 0);
  }
  
  try {
    auto inputDevices = manager_->get_available_input_devices();
    auto outputDevices = manager_->get_available_output_devices();
    
    Napi::Object result = Napi::Object::New(env);
    
    Napi::Array inputs = Napi::Array::New(env, inputDevices.size());
    for (size_t i = 0; i < inputDevices.size(); i++) {
      Napi::Object device = Napi::Object::New(env);
      device.Set("id", inputDevices[i]);
      device.Set("name", inputDevices[i]);
      device.Set("type", "input");
      inputs[i] = device;
    }
    result.Set("inputs", inputs);
    
    Napi::Array outputs = Napi::Array::New(env, outputDevices.size());
    for (size_t i = 0; i < outputDevices.size(); i++) {
      Napi::Object device = Napi::Object::New(env);
      device.Set("id", outputDevices[i]);
      device.Set("name", outputDevices[i]);
      device.Set("type", "output");
      outputs[i] = device;
    }
    result.Set("outputs", outputs);
    
    return result;
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to get devices: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value MidiDeviceManagerWrapper::OpenDevice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "MIDI device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected device ID and type").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    std::string deviceId = info[0].As<Napi::String>().Utf8Value();
    std::string type = info[1].As<Napi::String>().Utf8Value();
    
    bool success = false;
    if (type == "input") {
      success = manager_->set_input_device(deviceId);
    } else if (type == "output") {
      success = manager_->set_output_device(deviceId);
    } else {
      Napi::TypeError::New(env, "Device type must be 'input' or 'output'").ThrowAsJavaScriptException();
      return env.Null();
    }
    
    return Napi::Boolean::New(env, success);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to open device: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}

Napi::Value MidiDeviceManagerWrapper::CloseDevice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!manager_) {
    Napi::Error::New(env, "MIDI device manager not available").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Expected device ID and type").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  try {
    std::string type = info[1].As<Napi::String>().Utf8Value();
    
    bool success = false;
    if (type == "input") {
      success = manager_->set_input_device("");
    } else if (type == "output") {
      success = manager_->set_output_device("");
    } else {
      Napi::TypeError::New(env, "Device type must be 'input' or 'output'").ThrowAsJavaScriptException();
      return env.Null();
    }
    
    return Napi::Boolean::New(env, success);
  } catch (const std::exception& e) {
    Napi::Error::New(env, std::string("Failed to close device: ") + e.what()).ThrowAsJavaScriptException();
    return env.Null();
  }
}
