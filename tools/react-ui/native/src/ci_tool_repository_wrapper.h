#ifndef CI_TOOL_REPOSITORY_WRAPPER_H
#define CI_TOOL_REPOSITORY_WRAPPER_H

#include <napi.h>
#include "CIToolRepository.hpp"
#include "CIDeviceModel.hpp"
#include <memory>

class CIToolRepositoryWrapper : public Napi::ObjectWrap<CIToolRepositoryWrapper> {
public:
  static Napi::Object Init(Napi::Env env, Napi::Object exports);
  CIToolRepositoryWrapper(const Napi::CallbackInfo& info);

  std::unique_ptr<ci_tool::CIToolRepository>& GetRepository() { return repository_; }

private:
  static Napi::FunctionReference constructor;
  
  Napi::Value Initialize(const Napi::CallbackInfo& info);
  Napi::Value Shutdown(const Napi::CallbackInfo& info);
  Napi::Value SendDiscovery(const Napi::CallbackInfo& info);
  Napi::Value GetLogs(const Napi::CallbackInfo& info);
  Napi::Value ClearLogs(const Napi::CallbackInfo& info);
  Napi::Value GetMUID(const Napi::CallbackInfo& info);
  Napi::Value GetCIDeviceManager(const Napi::CallbackInfo& info);
  Napi::Value GetMidiDeviceManager(const Napi::CallbackInfo& info);
  
  std::unique_ptr<ci_tool::CIToolRepository> repository_;
};

#endif
