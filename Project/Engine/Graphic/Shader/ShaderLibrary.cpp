#include "ShaderLibrary.h"
#include "DX12/DX12Manager.h"
#include "ShaderCacheUtil.h"
#include "Utils/Func/UtilFunc.h"
#include <iostream>

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;

static const wchar_t* StageToProfile(ShaderType stage)
{
	switch (stage)
	{
		case ShaderType::VS: return L"vs_6_0";
		case ShaderType::PS: return L"ps_6_0";
		case ShaderType::GS: return L"gs_6_0";
		case ShaderType::HS: return L"hs_6_0";
		case ShaderType::DS: return L"ds_6_0";
		case ShaderType::CS: return L"cs_6_0";
		default:             return L"ps_6_0";
	}
}

void ShaderLibrary::Init()
{
	shaders_.clear();
	Tsumi::Utils::Logger::Info("[ShaderLibrary] Initialized");

	// DXC 初期化
	InitDXC();

	// 一括コンパイル
	CompileAllShader();
}

void ShaderLibrary::CompileAllShader()
{
	auto tryCompile = [&](const std::string& name, const ShaderLoadModule& desc)
	{
		try {
			Compile(name, desc);
		}
		catch (const std::exception& e) {
			std::wcerr << L"ShaderLibrary::CompileAllShader - failed '"
				<< Tsumi::Utils::Func::Utf8ToWstring(name)
				<< L"': " << e.what() << std::endl;
		}
		catch (...) {
			std::wcerr << L"ShaderLibrary::CompileAllShader - failed '"
				<< Tsumi::Utils::Func::Utf8ToWstring(name)
				<< L"': unknown error" << std::endl;
		}
	};

	// -------------------------------
	// 通常オブジェクト描画
	// -------------------------------
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/VS/Object3D_Static.VS.hlsl", "main" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/PS/Object3D_Static.PS.hlsl", "main" };
		tryCompile("Object3D", m);
	}

	// -------------------------------
	// Deferred : GBuffer Pass
	// -------------------------------
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/GBuffer.hlsl", "GBufferVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/GBuffer.hlsl", "GBufferPS" };
		tryCompile("GBuffer", m);
	}

	// -------------------------------
	// Deferred : Directional Lighting Pass
	// -------------------------------
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/LightingDirectional.hlsl", "FullscreenVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/LightingDirectional.hlsl", "LightingDirectionalPS" };
		tryCompile("LightingDirectional", m);
	}

	// -------------------------------
	// Deferred : DebugFullscreen Pass
	// -------------------------------
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Debug/DebugFullscreen.VS.hlsl", "main" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Debug/DebugFullscreen.PS.hlsl", "main" };
		tryCompile("DebugFullscreen", m);
	}

	// -------------------------------
	// Shadow : ShadowCaster (Depth Only)
	// -------------------------------
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Shadow/ShadowCaster.VS.hlsl", "main" };
		// PS は不要（Depth Only）
		tryCompile("ShadowCaster", m);
	}

	Tsumi::Utils::Logger::Info("[ShaderLibrary] CompileAllShader - completed (errors were logged per-shader if any)");
}

IDxcBlob* ShaderLibrary::Get(const std::string& name, ShaderType stage) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = shaders_.find(name);
	if (it == shaders_.end()) return nullptr;

	auto itStage = it->second.blob.find(stage);
	if (itStage == it->second.blob.end()) return nullptr;

	return itStage->second.Get();
}

bool ShaderLibrary::Has(const std::string& name) const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return shaders_.find(name) != shaders_.end();
}

HRESULT ShaderLibrary::Compile(const std::string& name, const ShaderLoadModule& module)
{
	if (module.sources.empty()) {
		throw std::runtime_error("ShaderLibrary::Compile - no sources provided for " + name);
	}

	ShaderBlob compiled;
	HRESULT hr = S_OK;

	for (const auto& kv : module.sources)
	{
		const ShaderType stage = kv.first;
		const ShaderStageDesc& stageDesc = kv.second;

		// file / entry（entry省略なら main）
		const std::wstring filePath = Tsumi::Utils::Func::Utf8ToWstring(stageDesc.file);
		const std::string entryUtf8 = stageDesc.entry.empty() ? "main" : stageDesc.entry;
		const std::wstring entryPoint = Tsumi::Utils::Func::Utf8ToWstring(entryUtf8);

		Tsumi::Utils::Logger::Info(
			"[ShaderLibrary] Begin CompileShader, path:{}, stage:{}, entry:{}\n",
			filePath, static_cast<int>(stage), Tsumi::Utils::Func::Utf8ToWstring(entryUtf8));

		// -------------------------------
		// HLSL 読み込み
		// -------------------------------
		ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
		hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &shaderSource);
		if (FAILED(hr) || !shaderSource) {
			Tsumi::Utils::Logger::Error("[ShaderLibrary] Failed to load shader file: {}\n", filePath);
			return hr;
		}

		DxcBuffer shaderSourceBuffer{};
		shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
		shaderSourceBuffer.Size = shaderSource->GetBufferSize();
		shaderSourceBuffer.Encoding = DXC_CP_UTF8;

		// -------------------------------
		// Compile
		// -------------------------------
		const wchar_t* profile = StageToProfile(stage);

		LPCWSTR arguments[] = {
			filePath.c_str(),
			L"-E", entryPoint.c_str(),  
			L"-T", profile,
			L"-Zi", L"-Qembed_debug",
			L"-Od",
			L"-Zpr"
		};

		ComPtr<IDxcResult> shaderResult;
		hr = dxcCompiler_->Compile(
			&shaderSourceBuffer,
			arguments,
			_countof(arguments),
			dxcIncludeHandler_.Get(),
			IID_PPV_ARGS(&shaderResult)
		);
		if (FAILED(hr) || !shaderResult) {
			Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC Compile failed: {} stage:{} entry:{}\n",
				filePath, static_cast<int>(stage), entryPoint);
			return hr;
		}

		// -------------------------------
		// エラー/警告
		// -------------------------------
		ComPtr<IDxcBlobUtf8> shaderError;
		shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

		if (shaderError && shaderError->GetStringLength() != 0)
		{
			// 既存の方針：警告も含めて出たら止める（あなたの実装に合わせる）
			Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC error/warning ({}):\n{}",
				filePath, Tsumi::Utils::Func::Utf8ToWstring(shaderError->GetStringPointer()));
			return E_FAIL;
		}

		// -------------------------------
		// 生成物
		// -------------------------------
		ComPtr<IDxcBlob> shaderBlob;
		hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
		if (FAILED(hr) || !shaderBlob) {
			Tsumi::Utils::Logger::Error(
				"[ShaderLibrary] DXC: failed to get compiled object for {}\n", filePath);
			return hr;
		}

		Tsumi::Utils::Logger::Info(
			"[ShaderLibrary] Compile Succeeded, path:{}, profile:{}, entry:{}\n",
			filePath, profile, entryPoint);

		compiled.blob[stage] = shaderBlob;
	}

	// 登録（スレッドセーフ）
	{
		std::lock_guard<std::mutex> lock(mutex_);
		shaders_[name] = std::move(compiled);
	}

	return S_OK;
}

HRESULT ShaderLibrary::InitDXC()
{
	HRESULT hr{};

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	if (FAILED(hr)) return hr;

	// 既存が IDxcCompiler3 を使ってる前提
	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	if (FAILED(hr)) return hr;

	hr = dxcUtils_->CreateDefaultIncludeHandler(&dxcIncludeHandler_);
	if (FAILED(hr)) return hr;

	return S_OK;
}
