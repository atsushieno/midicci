#include "ci_tool_repository_wrapper.h"

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
  
  return Napi::Boolean::New(env, true);
}

Napi::Value CIToolRepositoryWrapper::Shutdown(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::Boolean::New(env, true);
}

Napi::Value CIToolRepositoryWrapper::SendDiscovery(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  auto ci_manager = repository_->get_ci_device_manager();
  if (ci_manager) {
    repository_->log("Discovery inquiry sent", ci_tool::MessageDirection::Out);
  }
  
  return Napi::Boolean::New(env, true);
}

Napi::Value CIToolRepositoryWrapper::GetLogs(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  auto logs = repository_->get_logs();
  Napi::Array result = Napi::Array::New(env, logs.size());
  
  for (size_t i = 0; i < logs.size(); i++) {
    Napi::Object logEntry = Napi::Object::New(env);
    logEntry.Set("message", logs[i].message);
    logEntry.Set("direction", static_cast<int>(logs[i].direction));
    result[i] = logEntry;
  }
  
  return result;
}

Napi::Value CIToolRepositoryWrapper::ClearLogs(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  repository_->clear_logs();
  return Napi::Boolean::New(env, true);
}

Napi::Value CIToolRepositoryWrapper::GetMUID(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  
  if (!repository_) {
    Napi::TypeError::New(env, "Repository not initialized").ThrowAsJavaScriptException();
    return env.Null();
  }
  
  return Napi::Number::New(env, repository_->get_muid());
}
