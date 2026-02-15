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

enum class ShaderType
{
	VS,
	PS,
	GS,
	HS,
	DS,
	CS
};

struct ShaderStageDesc
{
	std::string file;
	std::string entry;
};

struct ShaderLoadModule
{
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

private:
	void CompileAllShader();
	HRESULT Compile(const std::string& name, const ShaderLoadModule& module);
	HRESULT InitDXC();

private:
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils_;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler_;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> dxcIncludeHandler_;

	std::unordered_map<std::string, ShaderBlob> shaders_;
	mutable std::mutex mutex_;
};

}