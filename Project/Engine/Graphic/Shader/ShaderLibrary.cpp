#include "ShaderLibrary.h"
#include "Utils/Func/UtilFunc.h"
#include <vector>

using namespace Tsumi::Graphic;
using namespace Microsoft::WRL;
using namespace tme;

// ------------------------------------------------------------
// ShaderType → Profile
// ------------------------------------------------------------
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

// ------------------------------------------------------------
// DXC Init
// ------------------------------------------------------------
HRESULT ShaderLibrary::InitDXC()
{
	HRESULT hr{};

	hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils_));
	if (FAILED(hr))
	{
		util::Logger::Error("[ShaderLibrary] Failed to create IDxcUtils");
		return hr;
	}

	hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler_));
	if (FAILED(hr))
	{
		util::Logger::Error("[ShaderLibrary] Failed to create IDxcCompiler");
		return hr;
	}

	hr = dxcUtils_->CreateDefaultIncludeHandler(&dxcIncludeHandler_);
	if (FAILED(hr))
	{
		util::Logger::Error("[ShaderLibrary] Failed to create IncludeHandler");
		return hr;
	}

	util::Logger::Info("[ShaderLibrary] DXC initialized");
	return S_OK;
}

// ------------------------------------------------------------
// Init
// ------------------------------------------------------------
void ShaderLibrary::Init()
{
	shaders_.clear();

	if (FAILED(InitDXC()))
	{
		util::Logger::Error("[ShaderLibrary] DXC init failed");
		return;
	}

	CompileAllShader();
}

IDxcBlob* ShaderLibrary::Get(const std::string& name, ShaderType stage) const
{
	std::lock_guard<std::mutex> lock(mutex_);

	auto it = shaders_.find(name);
	if (it == shaders_.end())
		return nullptr;

	auto itStage = it->second.blob.find(stage);
	if (itStage == it->second.blob.end())
		return nullptr;

	return itStage->second.Get();
}

// ------------------------------------------------------------
// Compile All
// ------------------------------------------------------------
void ShaderLibrary::CompileAllShader()
{
	util::Logger::Info("[ShaderLibrary] CompileAllShader start");

	auto tryCompile = [&](const std::string& name, const ShaderLoadModule& desc)
	{
		HRESULT hr = Compile(name, desc);
		if (FAILED(hr))
		{
			util::Logger::Error("[ShaderLibrary] Compile failed:", name);
		}
		else
		{
			util::Logger::Info("[ShaderLibrary] Compile succeeded:", name);
		}
	};

	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/DeferredGBuffer.hlsl", "GBufferVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/DeferredGBuffer.hlsl", "GBufferPS" };
		tryCompile("DeferredGBuffer", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/DeferredDirectionalLight.hlsl", "DirLightingVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/DeferredDirectionalLight.hlsl", "DirLightingPS" };
		tryCompile("DeferredDirectionalLight", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/DeferredPointLight.hlsl", "PointLightingVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/DeferredPointLight.hlsl", "PointLightingPS" };
		tryCompile("DeferredPointLight", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/DeferredSpotLight.hlsl", "SpotLightingVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/DeferredSpotLight.hlsl", "SpotLightingPS" };
		tryCompile("DeferredSpotLight", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Deferred/DeferredComposite.hlsl", "CompositeVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Deferred/DeferredComposite.hlsl", "CompositePS" };
		tryCompile("DeferredComposite", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Shadow/ShadowCaster.hlsl", "ShadowVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Shadow/ShadowCaster.hlsl", "ShadowPS" };
		tryCompile("ShadowCaster", m);
	}
	{
		ShaderLoadModule m;
		m.sources[ShaderType::VS] = { "Resources/Shaders/Debug/DeferredDebugView.hlsl", "DeferredDebugVS" };
		m.sources[ShaderType::PS] = { "Resources/Shaders/Debug/DeferredDebugView.hlsl", "DeferredDebugPS" };

		tryCompile("DeferredDebug", m);
	}


	util::Logger::Info("[ShaderLibrary] CompileAllShader end");
}

// ------------------------------------------------------------
// Compile
// ------------------------------------------------------------
HRESULT ShaderLibrary::Compile(const std::string& name, const ShaderLoadModule& module)
{
	ShaderBlob compiled;

	for (const auto& kv : module.sources)
	{
		const ShaderType stage = kv.first;
		const ShaderStageDesc& stageDesc = kv.second;

		std::wstring filePath =
			Utils::Func::Utf8ToWstring(stageDesc.file);

		std::wstring entry =
			Utils::Func::Utf8ToWstring(
			stageDesc.entry.empty() ? "main" : stageDesc.entry);

		util::Logger::Info(
			"[ShaderLibrary] Compiling:",
			filePath,
			L" Entry:", entry);

		ComPtr<IDxcBlobEncoding> source;
		HRESULT hr = dxcUtils_->LoadFile(filePath.c_str(), nullptr, &source);
		if (FAILED(hr))
		{
			util::Logger::Error(
				"[ShaderLibrary] Failed to load shader file:",
				filePath);
			return hr;
		}

		DxcBuffer buffer{};
		buffer.Ptr = source->GetBufferPointer();
		buffer.Size = source->GetBufferSize();
		buffer.Encoding = DXC_CP_UTF8;

		const wchar_t* profile = StageToProfile(stage);

		std::vector<std::wstring> wargs = {
			filePath,
			L"-E", entry,
			L"-T", profile,
			L"-Zi", L"-Qembed_debug",
			L"-Od",
			L"-Zpr",
			L"-I", L"Resources/Shaders"
		};

		std::vector<LPCWSTR> args;
		for (auto& s : wargs)
			args.push_back(s.c_str());

		ComPtr<IDxcResult> result;
		hr = dxcCompiler_->Compile(
			&buffer,
			args.data(),
			(UINT32)args.size(),
			dxcIncludeHandler_.Get(),
			IID_PPV_ARGS(&result));

		if (FAILED(hr))
		{
			util::Logger::Error("[ShaderLibrary] DXC Compile call failed");
			return hr;
		}

		HRESULT status{};
		result->GetStatus(&status);

		ComPtr<IDxcBlobUtf8> errors;
		result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);

		if (errors && errors->GetStringLength() > 0)
		{
			util::Logger::Warn(
				"[ShaderLibrary] DXC message:",
				Utils::Func::Utf8ToWstring(
				errors->GetStringPointer()));
		}

		if (FAILED(status))
		{
			util::Logger::Error("[ShaderLibrary] Shader compile failed:", name);
			return E_FAIL;
		}

		ComPtr<IDxcBlob> blob;
		hr = result->GetOutput(
			DXC_OUT_OBJECT,
			IID_PPV_ARGS(&blob),
			nullptr);

		if (FAILED(hr))
		{
			util::Logger::Error("[ShaderLibrary] Failed to get compiled object");
			return hr;
		}

		compiled.blob[stage] = blob;
	}

	{
		std::lock_guard<std::mutex> lock(mutex_);
		shaders_[name] = std::move(compiled);
	}

	return S_OK;
}
