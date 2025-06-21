#include <napi.h>
#include "ci_tool_repository_wrapper.h"
#include "ci_device_manager_wrapper.h"
#include "midi_device_manager_wrapper.h"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  CIToolRepositoryWrapper::Init(env, exports);
  CIDeviceManagerWrapper::Init(env, exports);
  MidiDeviceManagerWrapper::Init(env, exports);
  return exports;
}

NODE_API_MODULE(midicci_bridge, InitAll)
