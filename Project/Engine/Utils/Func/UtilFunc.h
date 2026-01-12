#pragma once
#include <Windows.h>
#include <string>
#include <filesystem>

namespace Tsumi::Utils::Func {
std::wstring Utf8ToWstring(const std::string& utf8);
std::string WstringToUtf8(const std::wstring& wstr);
std::string MakeKeyFromRoot(const std::string& root, const std::string& name);
}