#include "ci_device_manager_wrapper.h"

Napi::FunctionReference CIDeviceManagerWrapper::constructor;

Napi::Object CIDeviceManagerWrapper::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func = DefineClass(env, "CIDeviceManager", {
    InstanceMethod("getConnections", &CIDeviceManagerWrapper::GetConnections),
    InstanceMethod("setProfile", &CIDeviceManagerWrapper::SetProfile),
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
}

Napi::Value CIDeviceManagerWrapper::GetConnections(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  Napi::Array result = Napi::Array::New(env, 0);
  return result;
}

Napi::Value CIDeviceManagerWrapper::SetProfile(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}
