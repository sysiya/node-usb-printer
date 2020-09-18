#include <napi.h>

#include "addon.h"
#include "common.h"

using namespace Napi;

Value BuildObjectDevice(Env env, addon::Device device) {
  auto object = Object::New(env);
  object.Set("id", device.id);
  object.Set("parent", device.parent);
  object.Set("name", device.name);
  object.Set("path", device.path);
  return object;
}

Value Find(const CallbackInfo &info) {
  Env env = info.Env();
  auto list = Array::New(env);

  auto deviceList = addon::FindDeviceList(env);
  uint32_t index = 0;
  for (auto device : deviceList) {
    list.Set(index++, BuildObjectDevice(env, device));
  }

  return list;
}

Value WrinteData(const CallbackInfo &info) {
  auto env = info.Env();
  if (info.Length() == 0) {
    TypeError::New(env, std::string("require 3 arguments")).ThrowAsJavaScriptException();
    return Boolean::New(env, false);
  }
  if (!info[0].IsString()) {
    TypeError::New(env, std::string("first param 'devicePath' must be string")).ThrowAsJavaScriptException();
    return Boolean::New(env, false);
  }
  bool shouldRead = false;
  if (!info[1].IsString()) {
    TypeError::New(env, std::string("secrod param 'data' must be string")).ThrowAsJavaScriptException();
    return Boolean::New(env, false);
  }
  if (info.Length() > 2) {
    if (!info[2].IsBoolean()) {
      TypeError::New(env, std::string("third param 'shouldRead' must be boolean")).ThrowAsJavaScriptException();
      return Boolean::New(env, false);
    } else {
      shouldRead = info[2].As<Boolean>();
    }
  }
  auto devicePath = info[0].As<String>().Utf8Value();
  // std::cout << "input data:" << std::endl;
  // std::cout << "hex: " << ToHex(info[1].As<String>().Utf8Value()) << std::endl;
  // std::cout << "str: " << info[1].As<String>().Utf8Value() << std::endl;
  auto data = Utf8ToGbk(info[1].As<String>().Utf8Value().c_str());
  // std::cout << "gbk data:" << std::endl;
  // std::cout << "hex: " << ToHex(data) << " (" << data.length() << ")" << std::endl;
  // std::cout << "str: " << data << std::endl;
  auto response = addon::WriteData(env, devicePath, data.c_str(), data.length(), shouldRead);
  return Boolean::From(env, response);
}

ThreadSafeFunction tsfnAdded;
ThreadSafeFunction tsfnRemoved;

Value RegisterAddedCallback(const CallbackInfo &info) {
  auto env = info.Env();
  if (!info[0].IsFunction()) {
    TypeError::New(env, std::string("first param 'callback' must be function")).ThrowAsJavaScriptException();
    return Boolean::New(env, false);
  }
  tsfnAdded = ThreadSafeFunction::New(env, info[0].As<Function>(), "Added", 0, 1);
  addon::RegisterEventHandlerAdded([&](std::string devicePath) {
    tsfnAdded.BlockingCall(&devicePath, [](Env env, Function callback, std::string *devicePath) {
      callback.Call({String::New(env, devicePath->c_str())});
    });
  });
  return Boolean::New(env, true);
}

Value RegisterRemovedCallback(const CallbackInfo &info) {
  auto env = info.Env();
  if (!info[0].IsFunction()) {
    TypeError::New(env, std::string("first param 'callback' must be function")).ThrowAsJavaScriptException();
    return Boolean::New(env, false);
  }
  tsfnRemoved = ThreadSafeFunction::New(env, info[0].As<Function>(), "Removed", 0, 1);
  addon::RegisterEventHandlerRemoved([&](std::string devicePath) {
    tsfnRemoved.BlockingCall(&devicePath, [](Env env, Function callback, std::string *devicePath) {
      callback.Call({String::New(env, devicePath->c_str())});
    });
  });
  return Boolean::New(env, true);
}

Value StartMonitoring(const CallbackInfo &info) {
  auto env = info.Env();
  addon::Start();
  return env.Undefined();
}

Value StopMonitoring(const CallbackInfo &info) {
  auto env = info.Env();
  addon::Stop();
  tsfnAdded.Release();
  tsfnRemoved.Release();
  return env.Undefined();
}

Object Init(Env env, Object exports) {
  exports.Set("find", Function::New<Find>(env));
  exports.Set("writeData", Function::New<WrinteData>(env));
  exports.Set("registerAddedCallback", Function::New<RegisterAddedCallback>(env));
  exports.Set("registerRemovedCallback", Function::New<RegisterRemovedCallback>(env));
  exports.Set("startMonitoring", Function::New<StartMonitoring>(env));
  exports.Set("stopMonitoring", Function::New<StopMonitoring>(env));
  return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)