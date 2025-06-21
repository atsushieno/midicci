#ifndef MIDI_DEVICE_MANAGER_WRAPPER_H
#define MIDI_DEVICE_MANAGER_WRAPPER_H

#include <napi.h>
#include "MidiDeviceManager.hpp"
#include <memory>

class MidiDeviceManagerWrapper : public Napi::ObjectWrap<MidiDeviceManagerWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  static Napi::FunctionReference constructor;
  MidiDeviceManagerWrapper(const Napi::CallbackInfo& info);

  void SetManager(std::shared_ptr<ci_tool::MidiDeviceManager> manager) { manager_ = manager; }

private:
  Napi::Value GetDevices(const Napi::CallbackInfo& info);
  Napi::Value OpenDevice(const Napi::CallbackInfo& info);
  Napi::Value CloseDevice(const Napi::CallbackInfo& info);
  Napi::Value Initialize(const Napi::CallbackInfo& info);
  Napi::Value Shutdown(const Napi::CallbackInfo& info);
  
  std::shared_ptr<ci_tool::MidiDeviceManager> manager_;
};

#endif
