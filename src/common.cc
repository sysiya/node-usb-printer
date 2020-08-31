#include "common.h"

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

void ErrorHandler(string functionName) {
  auto message = GetLastErrorMessage();
  cout << functionName << " Error: " << message << endl;
}

string ToHex(string str) {
  ostringstream ret;

  for (string::size_type i = 0; i < str.length(); ++i) {
    ret << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << (int)str[i] << ' ';
  }

  return ret.str();
}

string Utf8ToGbk(const char *src_str) {
  int len = MultiByteToWideChar(CP_UTF8, 0, src_str, -1, NULL, 0);
  wchar_t *wszGBK = new wchar_t[len + 1];
  memset(wszGBK, 0, len * 2 + 2);
  MultiByteToWideChar(CP_UTF8, 0, src_str, -1, wszGBK, len);

  len = WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, NULL, 0, NULL, NULL);
  char *szGBK = new char[len + 1];
  memset(szGBK, 0, len + 1);
  WideCharToMultiByte(CP_ACP, 0, wszGBK, -1, szGBK, len, NULL, NULL);

  string strTemp(szGBK);
  if (wszGBK) delete[] wszGBK;
  if (szGBK) delete[] szGBK;
  return strTemp;
}

string GbkToUtf8(const char *src_str) {
  int len = MultiByteToWideChar(CP_ACP, 0, src_str, -1, NULL, 0);
  wchar_t *wstr = new wchar_t[len + 1];
  memset(wstr, 0, len + 1);
  MultiByteToWideChar(CP_ACP, 0, src_str, -1, wstr, len);

  len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
  char *str = new char[len + 1];
  memset(str, 0, len + 1);
  WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);

  string strTemp = str;
  if (wstr) delete[] wstr;
  if (str) delete[] str;
  return strTemp;
}
