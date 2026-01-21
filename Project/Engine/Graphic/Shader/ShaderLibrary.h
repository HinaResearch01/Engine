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

enum class ShaderType : uint8_t {
	VS,
	PS,
	GS,
	HS,
	DS,
	CS
};

// 1ステージ分の定義（同一hlslにVS/PSが共存してもOK）
struct ShaderStageDesc
{
	std::string file;   // 例: "Resources/Shaders/Deferred/GBuffer.hlsl"
	std::string entry;  // 例: "GBufferVS"（省略時は "main"）
};

struct ShaderLoadModule
{
	// ステージ → (file, entry)
	std::unordered_map<ShaderType, ShaderStageDesc> sources;
};

struct ShaderBlob
{
	std::unordered_map<ShaderType, Microsoft::WRL::ComPtr<IDxcBlob>> blob;
};

class ShaderLibrary {

private: // シングルトン
	ShaderLibrary() = default;
	~ShaderLibrary() = default;
	ShaderLibrary(const ShaderLibrary&) = delete;
	const ShaderLibrary operrator(const ShaderLibrary&) = delete;

public:
	/// <summary>
	/// インスタンス取得
	/// </summary>
	static ShaderLibrary* GetInstance() {
		static ShaderLibrary instance;
		return &instance;
	}

	/// <summary>
	/// 初期化処理
	/// </summary>
	void Init();

	IDxcBlob* Get(const std::string& name, ShaderType stage) const;
	bool Has(const std::string& name) const;

private:
	void CompileAllShader();
	HRESULT Compile(const std::string& name, const ShaderLoadModule& module);
	HRESULT InitDXC();

private:
	mutable std::mutex mutex_;
	std::unordered_map<std::string, ShaderBlob> shaders_;

	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler_;
};



}