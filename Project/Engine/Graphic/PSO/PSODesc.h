#pragma once

#include <vector>
#include <d3d12.h>
#include <d3dx12.h>
#include <cstdint>

namespace Tsumi::Graphic {

enum class BlendMode {
	Opaque,
	Alpha,
	Additive,
	Multiply
};

class PSODesc {

public:
	PSODesc();

	// RootSig
	void SetRootSignature(ID3D12RootSignature* rs);

	// Shader
	void SetVS(const D3D12_SHADER_BYTECODE& vs);
	void SetPS(const D3D12_SHADER_BYTECODE& ps);
	void ClearPS();

	// Formats
	void SetRTVFormats(UINT count, const DXGI_FORMAT* fmts);
	void SetDSVFormat(DXGI_FORMAT fmt);

	// States
	void SetRasterizer(const D3D12_RASTERIZER_DESC& rs);
	void SetBlend(BlendMode mode);
	void SetDepthStencil(const D3D12_DEPTH_STENCIL_DESC& dss);

	// Defaults helpers
	void SetCullMode(D3D12_CULL_MODE cull);
	void EnableDepth(bool enable);
	void SetDepthFunc(D3D12_COMPARISON_FUNC func);
	void SetDepthWrite(bool enable);

	// depth bias
	void SetDepthBias(int32_t bias);
	void SetSlopeScaledDepthBias(float bias);
	void SetDepthBiasClamp(float clamp);

	// IA
	void SetInputLayout(const std::vector<D3D12_INPUT_ELEMENT_DESC>& elems);
	void ClearInputLayout(); // fullscreen triangle 用
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE type);

	// Sample
	void SetSampleDesc(UINT count = 1, UINT quality = 0);

	// Build
	D3D12_GRAPHICS_PIPELINE_STATE_DESC BuildDesc() const;

private:
	ID3D12RootSignature* rootSig_ = nullptr;

	D3D12_SHADER_BYTECODE vs_{};
	D3D12_SHADER_BYTECODE ps_{};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC desc_{};

	// InputLayout の寿命管理
	std::vector<D3D12_INPUT_ELEMENT_DESC> inputElems_;
};

}
