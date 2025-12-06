#pragma once
#include <Windows.h>
#include <string>

namespace Tsumi::Utils {

std::wstring Utf8ToWstring(const std::string& utf8);
std::string WstringToUtf8(const std::wstring& wstr);

}