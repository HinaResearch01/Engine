#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include "Utils/Logger/UtilsLog.h"

#pragma comment(lib, "d3dcompiler.lib")

namespace Tsumi::Graphic {

enum class ShaderType { VS, PS, GS, HS, DS, CS };

struct ShaderData {
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    std::wstring entryPoint;
    ShaderType type;
};
struct ShaderLoadModule {
    std::unordered_map<ShaderType, std::wstring> sources;
};
struct ShaderBlob {
    std::unordered_map<ShaderType, Microsoft::WRL::ComPtr<ID3DBlob>> blob;
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
    ShaderBlob Get(const std::wstring& name) const;

    /// <summary>
    /// 読みこみ済み確認
    /// </summary>
    bool Has(const std::wstring& name) const;

private:

    /// <summary>
    /// コンパイル
    /// </summary>
    HRESULT Compile(const std::wstring& name, const ShaderLoadModule& desc);

private:
    std::unordered_map<std::wstring, ShaderBlob> shaders_;
    mutable std::mutex mutex_;
};


}