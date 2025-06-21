#ifndef CI_DEVICE_MANAGER_WRAPPER_H
#define CI_DEVICE_MANAGER_WRAPPER_H

#include <napi.h>
#include "CIDeviceManager.hpp"
#include "CIDeviceModel.hpp"
#include "midicci/profiles/MidiCIProfile.hpp"
#include <memory>

class CIDeviceManagerWrapper : public Napi::ObjectWrap<CIDeviceManagerWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::FunctionReference constructor;
  CIDeviceManagerWrapper(const Napi::CallbackInfo& info);

  void SetManager(std::shared_ptr<ci_tool::CIDeviceManager> manager) { manager_ = manager; }

private:
  Napi::Value GetConnections(const Napi::CallbackInfo& info);
  Napi::Value SetProfile(const Napi::CallbackInfo& info);
  Napi::Value SubscribeProperty(const Napi::CallbackInfo& info);
  Napi::Value UnsubscribeProperty(const Napi::CallbackInfo& info);
  Napi::Value RefreshPropertyValue(const Napi::CallbackInfo& info);
  Napi::Value CreateProperty(const Napi::CallbackInfo& info);
  Napi::Value UpdatePropertyMetadata(const Napi::CallbackInfo& info);
  Napi::Value UpdatePropertyValue(const Napi::CallbackInfo& info);
  Napi::Value RemoveProperty(const Napi::CallbackInfo& info);
  
  std::shared_ptr<ci_tool::CIDeviceManager> manager_;
};

#endif
