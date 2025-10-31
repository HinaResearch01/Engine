#include "PSOUtil.h"

D3D12_INPUT_ELEMENT_DESC Tsumi::Graphic::PSOUtil::SetUpInputElementDescs(LPCSTR SemanticName)
{
	D3D12_INPUT_ELEMENT_DESC inputElementDescs{};

	if (strcmp(SemanticName, "POSITION") == 0)
	{
		inputElementDescs.SemanticName = "POSITION";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "TEXCOORD") == 0)
	{
		inputElementDescs.SemanticName = "TEXCOORD";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "NORMAL") == 0)
	{
		inputElementDescs.SemanticName = "NORMAL";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "WORLDPOSITION") == 0)
	{
		inputElementDescs.SemanticName = "WORLDPOSITION";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "CAMERAPOSITION") == 0)
	{
		inputElementDescs.SemanticName = "CAMERAPOSITION";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "COLOR") == 0)
	{
		inputElementDescs.SemanticName = "COLOR";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs.InputSlot = 0;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "WEIGHT") == 0)
	{
		inputElementDescs.SemanticName = "WEIGHT";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
		inputElementDescs.InputSlot = 1;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "INDEX") == 0)
	{
		inputElementDescs.SemanticName = "INDEX";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32G32B32A32_SINT;
		inputElementDescs.InputSlot = 1;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}
	else if (strcmp(SemanticName, "INSTANCEID") == 0)
	{
		inputElementDescs.SemanticName = "INSTANCEID";
		inputElementDescs.SemanticIndex = 0;
		inputElementDescs.Format = DXGI_FORMAT_R32_UINT;
		inputElementDescs.InputSlot = 1;
		inputElementDescs.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	}

	return inputElementDescs;
}

D3D12_RENDER_TARGET_BLEND_DESC Tsumi::Graphic::PSOUtil::SetUpBlendState(BlendMode blendMode)
{
	D3D12_RENDER_TARGET_BLEND_DESC desc{};

	desc.RenderTargetWriteMask =
		D3D12_COLOR_WRITE_ENABLE_ALL;
	desc.BlendEnable = true;

	switch (blendMode)
	{
		case None:
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			break;

		case Add:
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_ONE;
			break;

		case Subtruct:
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
			desc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
			desc.DestBlend = D3D12_BLEND_ONE;
			break;

		case Multiply:
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlend = D3D12_BLEND_ZERO;
			desc.DestBlend = D3D12_BLEND_SRC_COLOR;
			break;

		case Screen:
			desc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
			desc.SrcBlendAlpha = D3D12_BLEND_ONE;
			desc.DestBlendAlpha = D3D12_BLEND_ZERO;
			desc.BlendOp = D3D12_BLEND_OP_ADD;
			desc.SrcBlend = D3D12_BLEND_INV_DEST_COLOR;
			desc.DestBlend = D3D12_BLEND_ONE;
			break;

		default:
			break;
	}

	return desc;
}
