#include "midi_device_manager_wrapper.h"

Napi::FunctionReference MidiDeviceManagerWrapper::constructor;

Napi::Object MidiDeviceManagerWrapper::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "MidiDeviceManager", {
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

Napi::Value MidiDeviceManagerWrapper::GetDevices(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Array result = Napi::Array::New(env, 0);
  return result;
}

Napi::Value MidiDeviceManagerWrapper::OpenDevice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Value MidiDeviceManagerWrapper::CloseDevice(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}
