#pragma once

#include <d3dx12.h>
#include <dxcapi.h>

namespace Tsumi::Graphic {

enum BlendMode {
	None,
	Add,
	Subtruct,
	Multiply,
	Screen,
};
}

namespace Tsumi::Graphic::PSOUtil {

D3D12_INPUT_ELEMENT_DESC SetUpInputElementDescs(LPCSTR SemanticName);

D3D12_RENDER_TARGET_BLEND_DESC SetUpBlendState(BlendMode blendMode);

}