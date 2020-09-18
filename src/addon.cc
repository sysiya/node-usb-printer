#include "addon.h"
#include "common.h"

using namespace std;

#define MAX_INTERFACE_DETAIL_DATA_SIZE (1024)
#define DEFAULT_NUMBER_OF_BYTES_READ (1024)

namespace addon {

string WcharToChar(const wchar_t *wp) {
  string str;
  int len = wcslen(wp) * sizeof(wchar_t);
  char *m_char = new char[len];
  setlocale(LC_ALL, "");
  wcstombs(m_char, wp, len);
  str = m_char;
  delete[] m_char;
  return str;
}

// Create a string with last error message
string GetLastErrorMessage() {
  DWORD error = GetLastError();
  if (error) {
    LPVOID lpMsgBuf;
    DWORD bufLen =
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                      error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
    if (bufLen) {
      LPCSTR lpMsgStr = (LPCSTR)lpMsgBuf;
      std::string result(lpMsgStr, lpMsgStr + bufLen);

      LocalFree(lpMsgBuf);

      return result;
    }
  }
  return string();
}

void ErrorHandler(string functionName, Napi::Env env = NULL) {
  auto message = GetLastErrorMessage();
  cout << functionName << " Error: " << message << endl;
  if (env != NULL)
    Napi::Error::New(env, functionName + " called: " + GbkToUtf8(message.c_str())).ThrowAsJavaScriptException();
  return;
}

wstring GetDeviceProperty(HDEVINFO hDeviceInfoSet, PSP_DEVINFO_DATA pDeviceInfoData, const DEVPROPKEY *property,
                          const wstring propertyName = L"") {
  DEVPROPTYPE propertyType;
  DWORD requiredSize;
  BOOL success;

  success =
      SetupDiGetDevicePropertyW(hDeviceInfoSet, pDeviceInfoData, property, &propertyType, NULL, 0, &requiredSize, 0);
  if (!success && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    ErrorHandler("SetupDiGetDevicePropertyW");
    return wstring();
  }

  wchar_t *buffer = new wchar_t[requiredSize];
  if (!SetupDiGetDevicePropertyW(hDeviceInfoSet, pDeviceInfoData, property, &propertyType, (PBYTE)buffer, requiredSize,
                                 NULL, 0)) {
    ErrorHandler("SetupDiGetDevicePropertyW");
    return wstring();
  }

  auto value = wstring(reinterpret_cast<wchar_t const *>(buffer), requiredSize / sizeof(wchar_t));
  delete[] buffer;

  if (propertyName != L"") {
    ios_base::sync_with_stdio(false);
    locale loc("zh_CN");
    wcout.imbue(loc);
    wcout << propertyName << L": " << value << endl;
  }

  return value;
}

// define device interface guid for usb printer which using the driver "usbprint.sys"
// USB Printing Support 28d78fad-5a12-11d1-ae5b-0000f803a8c2
const GUID GUID_DEVINTERFACE_USBPRINT = {0x28d78fad, 0x5a12, 0x11D1, 0xae, 0x5b, 0x00, 0x00, 0xf8, 0x03, 0xa8, 0xc2};

// find all usb printers which has connected with os
list<Device> FindDeviceList(Napi::Env env) {
  list<Device> deviceList;
  GUID classGuid = GUID_DEVINTERFACE_USBPRINT;

  // handle to a device infomation set that contains requested device infomation element
  HDEVINFO hDeviceInfoSet = SetupDiGetClassDevs(&classGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if (hDeviceInfoSet == INVALID_HANDLE_VALUE) {
    ErrorHandler("SetupDiGetClassDevs", env);
    return deviceList;
  }

  SP_DEVINFO_DATA spDeviceInfoData;
  for (auto nIndex = 0, hasNext = TRUE; hasNext; ++nIndex) {
    spDeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    if (!SetupDiEnumDeviceInfo(hDeviceInfoSet, nIndex, &spDeviceInfoData)) {
      if (GetLastError() == ERROR_NO_MORE_ITEMS) return deviceList;
      ErrorHandler("SetupDiEnumDeviceInfo", env);
      continue;
    }

    DEVPROPTYPE propertyType;
    BOOL success = FALSE;
    DWORD requiredSize;

    // get the string that was reported by the bus driver for the device instance
    auto deviceName = GetDeviceProperty(hDeviceInfoSet, &spDeviceInfoData, &DEVPKEY_Device_BusReportedDeviceDesc);
    auto deviceId = GetDeviceProperty(hDeviceInfoSet, &spDeviceInfoData, &DEVPKEY_Device_InstanceId);
    auto deviceParent = GetDeviceProperty(hDeviceInfoSet, &spDeviceInfoData, &DEVPKEY_Device_Parent);

    SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
    deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
    if (!SetupDiEnumDeviceInterfaces(hDeviceInfoSet, NULL, &classGuid, nIndex, &deviceInterfaceData)) {
      continue;
    }

    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
    pDeviceInterfaceDetailData =
        (PSP_DEVICE_INTERFACE_DETAIL_DATA)GlobalAlloc(LMEM_ZEROINIT, MAX_INTERFACE_DETAIL_DATA_SIZE);
    pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    if (!SetupDiGetInterfaceDeviceDetail(hDeviceInfoSet, &deviceInterfaceData, pDeviceInterfaceDetailData,
                                         MAX_INTERFACE_DETAIL_DATA_SIZE, NULL, NULL)) {
      ErrorHandler("SetupDiGetInterfaceDeviceDetail");
      continue;
    }
    Device device;
    device.id = WcharToChar(deviceId.c_str());
    device.parent = WcharToChar(deviceParent.c_str());
    device.name = WcharToChar(deviceName.c_str());
    device.path = pDeviceInterfaceDetailData->DevicePath;
    deviceList.push_back(device);

    GlobalFree(pDeviceInterfaceDetailData);
  }

  SetupDiDestroyDeviceInfoList(hDeviceInfoSet);

  return deviceList;
}

// wirte data into the usb printer associated with the specified device path.
string WriteData(Napi::Env env, string devicePath, const char *data, size_t size, bool shouldRead = false) {
  HANDLE hFile =
      CreateFile(devicePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    ErrorHandler("CreateFile", env);
    return string();
  }

  DWORD numberOfBytesWritten;
  if (!WriteFile(hFile, data, size, &numberOfBytesWritten, NULL)) {
    ErrorHandler("WriteFile", env);
    CloseHandle(hFile);
    return string();
  }

  if (!shouldRead) {
    CloseHandle(hFile);
    return string();
  }

  string result;
  result.resize(DEFAULT_NUMBER_OF_BYTES_READ);
  DWORD numberOfBytesRead;
  if (!ReadFile(hFile, &result[0], DEFAULT_NUMBER_OF_BYTES_READ, &numberOfBytesRead, NULL)) {
    ErrorHandler("ReadFile", env);
  }
  if (numberOfBytesRead != 0) {
    result.resize(numberOfBytesRead);
  } else {
    result.resize(1);
    result[0] = '\x00';
  }

  CloseHandle(hFile);

  return result;
}

// PnP Manager

#define WND_CLASS_NAME "PnP Manager"

auto isMonitoring = false;
std::thread thread_message_loop;
HWND hWnd;

std::function<void(string)> DeviceArrivalHandler = nullptr;
std::function<void(string)> DeviceRemovedHandler = nullptr;

bool DoRegisterDeviceInterfaceToHwnd(IN GUID interfaceClassGuid, IN HWND hWnd, OUT HDEVNOTIFY *hDeviceNotify) {
  DEV_BROADCAST_DEVICEINTERFACE NotificationFilter;

  ZeroMemory(&NotificationFilter, sizeof(NotificationFilter));
  NotificationFilter.dbcc_size = sizeof(DEV_BROADCAST_DEVICEINTERFACE);
  NotificationFilter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  NotificationFilter.dbcc_classguid = interfaceClassGuid;

  *hDeviceNotify = RegisterDeviceNotification(hWnd,                        // events recipient
                                              &NotificationFilter,         // type of device
                                              DEVICE_NOTIFY_WINDOW_HANDLE  // type of recipient handle
  );

  if (NULL == *hDeviceNotify) {
    ErrorHandler("RegisterDeviceNotification");
    return false;
  }

  return true;
}

INT_PTR WINAPI WinProcCallback(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT lRet = 1;
  static HDEVNOTIFY hDeviceNotify;

  switch (message) {
    case WM_CREATE:
      if (!DoRegisterDeviceInterfaceToHwnd(GUID_DEVINTERFACE_USBPRINT, hWnd, &hDeviceNotify)) {
        ErrorHandler("DoRegisterDeviceInterfaceToHwnd");
        ExitProcess(1);
      }
      break;

    case WM_DEVICECHANGE: {
      if (wParam != DBT_DEVICEARRIVAL && wParam != DBT_DEVICEREMOVECOMPLETE) break;

      auto lpDevBroadcastHdr = (PDEV_BROADCAST_HDR)lParam;
      if (lpDevBroadcastHdr->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) break;

      auto lpDevBroadcastDeviceInterface = (PDEV_BROADCAST_DEVICEINTERFACE)lParam;
      auto devicePath = lpDevBroadcastDeviceInterface->dbcc_name;

      if (wParam == DBT_DEVICEARRIVAL) {
        if (DeviceArrivalHandler != nullptr) {
          DeviceArrivalHandler(lpDevBroadcastDeviceInterface->dbcc_name);
        }
      } else if (wParam == DBT_DEVICEREMOVECOMPLETE) {
        if (DeviceRemovedHandler != nullptr) {
          DeviceRemovedHandler(lpDevBroadcastDeviceInterface->dbcc_name);
        }
      }
    } break;

    case WM_CLOSE:
      if (!UnregisterDeviceNotification(hDeviceNotify)) {
        ErrorHandler("UnregisterDeviceNotification");
      }
      DestroyWindow(hWnd);
      break;

    case WM_DESTROY:
      PostQuitMessage(0);
      break;

    default:
      // Send all other messages on to the default windows handler.
      lRet = DefWindowProc(hWnd, message, wParam, lParam);
      break;
  }

  return lRet;
}

bool InitWindowClass() {
  WNDCLASSEX wndClass;
  wndClass.cbSize = sizeof(WNDCLASSEX);
  wndClass.style = NULL;
  wndClass.hInstance = reinterpret_cast<HINSTANCE>(GetModuleHandle(NULL));
  wndClass.lpfnWndProc = reinterpret_cast<WNDPROC>(WinProcCallback);
  wndClass.cbClsExtra = 0;
  wndClass.cbWndExtra = 0;
  wndClass.hIcon = NULL;
  wndClass.hbrBackground = NULL;
  wndClass.hCursor = NULL;
  wndClass.lpszClassName = WND_CLASS_NAME;
  wndClass.lpszMenuName = NULL;
  wndClass.hIconSm = NULL;

  if (!RegisterClassEx(&wndClass)) {
    ErrorHandler("RegisterClassEx");
    return false;
  }
  return true;
}

void MessagePump(HWND hWnd) {
  MSG msg;

  // Get all messages for any window that belongs to this thread
  int retVal;
  while ((retVal = GetMessage(&msg, hWnd, 0, 0)) != 0) {
    if (retVal == -1) {
      break;
    } else {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

bool ThreadHandler() {
  if (!InitWindowClass()) {
    return false;
  }

  hWnd = CreateWindowEx(NULL, WND_CLASS_NAME, "PnP Manager Window", NULL, 0, 0, 0, 0, NULL, NULL,
                        (HINSTANCE)GetModuleHandle(NULL), NULL);
  if (hWnd == NULL) {
    ErrorHandler("CreateWindowEx");
    return false;
  }

  ShowWindow(hWnd, SW_HIDE);
  UpdateWindow(hWnd);

  // The message pump loops until the window is destroyed.
  MessagePump(hWnd);

  return true;
}

bool isAddedRegistered = false;
void RegisterEventHandlerAdded(function<void(string)> handler) {
  if (isAddedRegistered) return;
  DeviceArrivalHandler = handler;
  isAddedRegistered = true;
}

bool isRemovedRegistered = false;
void RegisterEventHandlerRemoved(function<void(string)> handler) {
  if (isRemovedRegistered) return;
  DeviceRemovedHandler = handler;
  isRemovedRegistered = true;
}

void Start() {
  if (isMonitoring) return;
  isMonitoring = true;

  thread_message_loop = std::thread(ThreadHandler);
  thread_message_loop.detach();
}

void Stop() {
  if (!isMonitoring) return;
  isMonitoring = false;

  if (hWnd != NULL) {
    PostMessage((HWND)hWnd, WM_CLOSE, NULL, NULL);
  }

  DeviceArrivalHandler = nullptr;
  DeviceRemovedHandler = nullptr;
  isAddedRegistered = false;
  isRemovedRegistered = false;
}

}  // namespace addon