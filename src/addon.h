#include <napi.h>

#include <Windows.h>
#include <SetupAPI.h>
#include <initguid.h>
#include <devpkey.h>
#include <Dbt.h>

#include <iostream>
#include <list>
#include <string>
#include <thread>
#include <functional>

namespace addon {

typedef struct Device {
  // the instance id of the usb device
  std::string id;
  // the instance id of the parent devnode
  std::string parent;
  // the name of the usb device, that is reported by usb bus.
  std::string name;
  // the path of the usb device, that is used to find device.
  std::string path;
} Device;

std::list<Device> FindDeviceList(Napi::Env env);

std::string WriteData(Napi::Env env, std::string devicePath, const char *data, size_t size, bool shouldRead);

void RegisterEventHandlerAdded(std::function<void(std::string)> handler);

void RegisterEventHandlerRemoved(std::function<void(std::string)> handler);

void Start();

void Stop();

}  // namespace addon