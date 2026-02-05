#include "RootSignatureLibrary.h"
#include "DX12/DX12Manager.h"
#include "RootSignatureIndex.h"
#include "Utils/Func/UtilFunc.h"
#include <d3dx12.h>
#include <stdexcept>
#include <locale>

using namespace Tsumi::Graphic;
using Microsoft::WRL::ComPtr;

RootSignatureLibrary::RootSignatureLibrary()
{
	dx12Mgr_ = Tsumi::DX12::DX12Manager::GetInstance();
}

void RootSignatureLibrary::Init()
{
	// 生成と登録
	CreateObject3D();
	CreateGBuffer();
	CreateLightingDirectional();
	CreateDebugFullScreen();
	CreateShadowCaster();
}

void RootSignatureLibrary::Register(const std::string& name, RootSignatureDesc& rs)
{
	D3D12_ROOT_SIGNATURE_DESC desc = rs.BuildDesc();
	RegisterFromDesc(name, desc, rs.GetStaticSamplers());
}

ID3D12RootSignature* RootSignatureLibrary::Get(const std::string& name) const
{
	std::lock_guard<std::mutex> lk(mutex_);
	auto it = rootSigs_.find(name);
	if (it == rootSigs_.end()) return nullptr;
	return it->second.Get();
}

bool RootSignatureLibrary::Has(const std::string& name) const
{
	std::lock_guard<std::mutex> lk(mutex_);
	return rootSigs_.find(name) != rootSigs_.end();
}

void RootSignatureLibrary::RegisterFromDesc(const std::string& name, const D3D12_ROOT_SIGNATURE_DESC& desc, const std::vector<D3D12_STATIC_SAMPLER_DESC>& samplers)
{
	// descをコピーして静的サンプラーを付与可能にする
	D3D12_ROOT_SIGNATURE_DESC descCopy = desc;
	if (!samplers.empty()) {
		descCopy.NumStaticSamplers = static_cast<UINT>(samplers.size());
		descCopy.pStaticSamplers = samplers.data();
	}
	else {
		descCopy.NumStaticSamplers = 0;
		descCopy.pStaticSamplers = nullptr;
	}

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;

	// ルートシグネチャをシリアライズ
	HRESULT hr = D3D12SerializeRootSignature(
		&descCopy,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&serializedRootSig,
		&errorBlob
	);

	// 名前を安全に wstring に変換
	std::wstring wname = Utils::Func::Utf8ToWstring(name);

	if (FAILED(hr)) {
		if (errorBlob) {
			std::string err(
				static_cast<const char*>(errorBlob->GetBufferPointer()),
				errorBlob->GetBufferSize());

			std::wstring werr = Utils::Func::Utf8ToWstring(err);

			Utils::Logger::Error(
				"RootSignatureLibrary::Register serialize error",
				"name:", wname,
				"message:", werr);
		}
		else {
			Utils::Logger::Error(
				"RootSignatureLibrary::Register serialize failed",
				"name:", wname,
				"hr:", static_cast<unsigned>(hr));
		}
		throw std::runtime_error("RootSignature serialize failed: " + name);
	}

	// GPU上にルートシグネチャを作成
	ComPtr<ID3D12RootSignature> rootSig;
	hr = dx12Mgr_->GetDevice()->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(&rootSig)
	);

	if (FAILED(hr) || !rootSig) {
		Utils::Logger::Error(
			"RootSignatureLibrary::Register - CreateRootSignature に失敗 '{}', hr=0x{:08X}\n",
			wname, static_cast<unsigned>(hr));
		throw std::runtime_error("RootSignature create failed: " + name);
	}

	// 登録処理（スレッドセーフ）
	{
		std::lock_guard<std::mutex> lk(mutex_);
		rootSigs_[name] = rootSig;
	}

	Utils::Logger::Info(
		"RootSignatureLibrary::Register - 登録完了 ", wname);
}

void RootSignatureLibrary::CreateObject3D()
{
	RootSignatureDesc rs;

	// VS
	rs.AddCBV(0, D3D12_SHADER_VISIBILITY_VERTEX); // Camera (b0)
	rs.AddCBV(1, D3D12_SHADER_VISIBILITY_VERTEX); // Transform (b1)

	// PS
	rs.AddCBV(2, D3D12_SHADER_VISIBILITY_PIXEL); // Material (b2)
	rs.AddSRVRange(0, 1, D3D12_SHADER_VISIBILITY_PIXEL); // Texture Albedo (t0) 1個

	// Sampler
	// s0 : Linear Wrap (PS)
	rs.AddStaticSampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0,
		D3D12_SHADER_VISIBILITY_PIXEL
	);
	// s1 : Point Clamp (PS)
	rs.AddStaticSampler(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0,
		D3D12_SHADER_VISIBILITY_PIXEL
	);

	rs.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	Register("Object3D", rs);
}

void RootSignatureLibrary::CreateGBuffer()
{
	RootSignatureDesc rs;

	// VS
	rs.AddCBVRange(0, 1, D3D12_SHADER_VISIBILITY_VERTEX); // b0  CameraCB
	rs.AddCBVRange(10, 1, D3D12_SHADER_VISIBILITY_VERTEX); // b10 ObjectCB

	// Material
	rs.AddCBVRange(20, 1, D3D12_SHADER_VISIBILITY_VERTEX); // b20 MaterialUVCB
	rs.AddCBVRange(21, 1, D3D12_SHADER_VISIBILITY_PIXEL);  // b21 MaterialParamsCB

	// PS
	rs.AddSRVRange(0, 1, D3D12_SHADER_VISIBILITY_PIXEL);   // t0  AlbedoSRV

	rs.AddStaticSampler(
		0,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0,
		D3D12_SHADER_VISIBILITY_PIXEL
	);

	rs.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Register("GBuffer", rs);
}

void RootSignatureLibrary::CreateLightingDirectional()
{
	RootSignatureDesc rs;

	// PS
	rs.AddCBVRange(0, 1, D3D12_SHADER_VISIBILITY_PIXEL);   // b0  CameraCB
	rs.AddCBVRange(30, 1, D3D12_SHADER_VISIBILITY_PIXEL);  // b30 DirectionalLightCB
	rs.AddSRVRange(10, 4, D3D12_SHADER_VISIBILITY_PIXEL);  // t10..t13 GBuffer

	// Sampler (s1 : PointClamp)
	rs.AddStaticSampler(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0,
		D3D12_SHADER_VISIBILITY_PIXEL
	);

	rs.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE);

	Register("LightingDirectional", rs);
}

void Tsumi::Graphic::RootSignatureLibrary::CreateDebugFullScreen()
{
	RootSignatureDesc rs;

	// PS
	rs.AddCBVRange(0, 1, D3D12_SHADER_VISIBILITY_PIXEL);    // [0] b0  CameraCB
	rs.AddCBVRange(50, 1, D3D12_SHADER_VISIBILITY_PIXEL);   // [1] b50 DebugCB
	rs.AddSRVRange(10, 4, D3D12_SHADER_VISIBILITY_PIXEL);   // [2] t10..t13 GBuffer

	// Sampler (s1 : PointClamp)
	rs.AddStaticSampler(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0,
		D3D12_SHADER_VISIBILITY_PIXEL
	);

	rs.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_NONE);

	Register("DebugFullScreen", rs);
}

void RootSignatureLibrary::CreateShadowCaster()
{
	RootSignatureDesc rs;

	// VS only
	rs.AddCBVRange(0, 1, D3D12_SHADER_VISIBILITY_VERTEX); // b0 ShadowCasterCB
	rs.AddCBVRange(1, 1, D3D12_SHADER_VISIBILITY_VERTEX); // b1 Object Transform

	rs.SetFlags(D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Register("ShadowCaster", rs);
}
