#ifndef _COMMON_H_
#define _COMMON_H_

#include <Windows.h>

#include <sstream>
#include <iostream>
#include <iomanip>
#include <list>
#include <string>

using namespace std;

string GetLastErrorMessage();
void ErrorHandler(string functionName);
string ToHex(string str);
string Utf8ToGbk(const char *src_str);
string GbkToUtf8(const char *src_str);

#endif