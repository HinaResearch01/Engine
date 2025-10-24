#pragma once

#include <string>
#include <Windows.h>
#include <format>
#include <sstream>
#include <iomanip>

namespace Tsumi::Utils {

inline void Log(const std::string& message) {
    std::string tagged = "[Tsumi] " + message;
    OutputDebugStringA(tagged.c_str());
}

inline void Log(const std::wstring& message) {
    std::wstring tagged = L"[Tsumi] " + message;
    OutputDebugStringW(tagged.c_str());
}

template<typename T>
inline void LogPtr(const wchar_t* name, const T* p) {
    std::wstringstream ss;
    ss << L"[Tsumi] " << name
        << L" = 0x" << std::uppercase << std::hex
        << reinterpret_cast<uintptr_t>(p)
        << std::endl;
    OutputDebugStringW(ss.str().c_str());
}

inline void LogHandle(const wchar_t* name, SIZE_T value) {
    std::wstringstream ss;
    ss << L"[Tsumi] " << name
        << L" = 0x" << std::uppercase << std::hex
        << std::setw(16) << std::setfill(L'0') << value
        << std::endl;
    OutputDebugStringW(ss.str().c_str());
}

}