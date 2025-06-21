#ifndef CI_DEVICE_MANAGER_WRAPPER_H
#define CI_DEVICE_MANAGER_WRAPPER_H

#include <napi.h>
#include "CIDeviceManager.hpp"
#include <memory>

class CIDeviceManagerWrapper : public Napi::ObjectWrap<CIDeviceManagerWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  CIDeviceManagerWrapper(const Napi::CallbackInfo& info);

private:
  static Napi::FunctionReference constructor;
  
  Napi::Value GetConnections(const Napi::CallbackInfo& info);
  Napi::Value SetProfile(const Napi::CallbackInfo& info);
  
  std::shared_ptr<ci_tool::CIDeviceManager> manager_;
};

#endif
