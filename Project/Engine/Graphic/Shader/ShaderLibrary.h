#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>
#include <format>
#include <iostream>
#include <map>
#include <memory>
#include <unordered_map>
#include <mutex>
#pragma comment(lib, "dxcompiler.lib")

#include "Utils/Logger/Logger.h"

namespace Tsumi::Graphic {

enum class ShaderType { VS, PS, GS, HS, DS, CS };

struct ShaderLoadModule {
    std::unordered_map<ShaderType, std::wstring> sources;
};
struct ShaderBlob {
    std::unordered_map<ShaderType, Microsoft::WRL::ComPtr<IDxcBlob>> blob;
};

/* シェーダー管理 */
class ShaderLibrary {

private: // シングルトン
    ShaderLibrary() = default;
    ~ShaderLibrary() = default;
    ShaderLibrary(const ShaderLibrary&) = delete;
    const ShaderLibrary& operator=(const ShaderLibrary&) = delete;

public:
    /// <summary>
    /// インスタンスの取得
    /// </summary>
    static ShaderLibrary* GetInstance() {
        static ShaderLibrary instance;
        return &instance;
    }

    /// <summary>
    /// 初期化処理
    /// </summary>
    void Init();

    /// <summary>
    /// 前者シェーダー一括読み込み
    /// </summary>
    void CompileAllShader();

    /// <summary>
    /// 取得
    /// </summary>
    IDxcBlob* Get(const std::wstring& name, ShaderType stage) const;

    /// <summary>
    /// 読みこみ済み確認
    /// </summary>
    bool Has(const std::wstring& name) const;

private:

    /// <summary>
    /// コンパイル
    /// </summary>
    HRESULT Compile(const std::wstring& name, const ShaderLoadModule& desc);

    /// <summary>
    /// DXCの初期化処理
    /// </summary>
    HRESULT InitDXC();

private:
    std::unordered_map<std::wstring, ShaderBlob> shaders_;
    mutable std::mutex mutex_;

    Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
    Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler_;
};


}