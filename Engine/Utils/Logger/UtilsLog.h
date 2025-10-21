#pragma once

#include <string>
#include <Windows.h>

namespace Tsumi::Utils {

inline void Log(const std::string& message) {
    OutputDebugStringA(message.c_str());
}

// wstring 版もあると便利
inline void Log(const std::wstring& message) {
    OutputDebugStringW(message.c_str());
}

}