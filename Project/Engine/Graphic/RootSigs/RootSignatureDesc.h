#pragma once
#include <vector>
#include <d3d12.h>
#include <d3dx12.h>

namespace Tsumi::Graphic {

class RootSignatureDesc {
public:
	// Root CBV
	void AddCBV(UINT shaderRegister, D3D12_SHADER_VISIBILITY visibility, UINT space = 0);

	// DescriptorTable
	void AddCBVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space = 0);
	void AddSRVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space = 0);
	void AddUAVRange(UINT baseRegister, UINT count, D3D12_SHADER_VISIBILITY visibility, UINT space = 0);

	void AddStaticSampler(UINT shaderRegister,
						  D3D12_FILTER filter,
						  D3D12_TEXTURE_ADDRESS_MODE address,
						  UINT space = 0,
						  D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_PIXEL);

	void SetFlags(D3D12_ROOT_SIGNATURE_FLAGS flags);

	D3D12_ROOT_SIGNATURE_DESC BuildDesc();

	const std::vector<D3D12_STATIC_SAMPLER_DESC>& GetStaticSamplers() const { return staticSamplers_; }

private:
	std::vector<CD3DX12_ROOT_PARAMETER> params_;

	std::vector<std::unique_ptr<CD3DX12_DESCRIPTOR_RANGE>> ownedRanges_;

	std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers_;
	D3D12_ROOT_SIGNATURE_FLAGS flags_ = D3D12_ROOT_SIGNATURE_FLAG_NONE;
};

}
