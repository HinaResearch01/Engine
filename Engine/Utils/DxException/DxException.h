#pragma once
#include <windows.h>
#include <exception>
#include <string>
#include <format> 
#include <comdef.h>
#include <codecvt> 
#include <locale> 

namespace Tsumi::DX12 {

// =============================================================
//  DxException : DirectX用の例外クラス
// =============================================================
class DxException : public std::exception {
public:
    DxException(HRESULT hr, const std::string& funcName, const std::string& filename, int line)
        : hr_(hr)
    {
        // HRESULT からエラーメッセージを取得
        _com_error err(hr);
        std::wstring wmsg = err.ErrorMessage();
        std::string msg = WideToUTF8(wmsg);

        message_ = std::format(
            "[DirectX Error]\n"
            "Function: {}\n"
            "File: {}\n"
            "Line: {}\n"
            "HRESULT: 0x{:08X}\n"
            "Message: {}\n",
            funcName, filename, line, static_cast<unsigned int>(hr), msg);
    }

    const char* what() const noexcept override {
        return message_.c_str();
    }

    HRESULT ErrorCode() const noexcept { return hr_; }


private:

    static std::string WideToUTF8(const std::wstring& wstr)
    {
        if (wstr.empty()) return {};

        int sizeNeeded = WideCharToMultiByte(
            CP_UTF8,            // UTF-8 へ変換
            0,
            wstr.c_str(),
            (int)wstr.size(),
            nullptr,
            0,
            nullptr,
            nullptr);

        std::string result(sizeNeeded, 0);
        WideCharToMultiByte(
            CP_UTF8,
            0,
            wstr.c_str(),
            (int)wstr.size(),
            result.data(),
            sizeNeeded,
            nullptr,
            nullptr);

        return result;
    }

private:
    HRESULT hr_;
    std::string message_;
};

// =============================================================
//  ThrowIfFailed : HRESULTのチェック関数
// =============================================================
inline void ThrowIfFailed(HRESULT hr, const std::string& funcName, const std::string& file, int line)
{
    if (FAILED(hr)) {
        throw DxException(hr, funcName, file, line);
    }
}

// =============================================================
//  マクロ：自動で関数名とファイル・行番号を渡す
// =============================================================
#define DX_CALL(x) ThrowIfFailed((x), #x, __FILE__, __LINE__)

}