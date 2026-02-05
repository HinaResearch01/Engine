#pragma once
#include <cstdint>

namespace Tsumi::Graphic::RootIndex {

template<class TRootEnum>
constexpr uint32_t ToRoot(TRootEnum e) {
	return static_cast<uint32_t>(e);
}


/// ============================
/// Object3D
/// ============================
enum class Root_Object3D : uint32_t {
	CameraCB = 0, // b0 VS
	TransformCB = 1, // b1 VS
	MaterialCB = 2, // b2 PS (旧)
	AlbedoSRV = 3, // t0
};

/// ============================
/// GBuffer
/// ============================
enum class Root_GBuffer : uint32_t {
	CameraCB = 0, // b0 VS
	ObjectCB = 1, // b10 VS
	MaterialUVCB = 2, // b20
	MaterialParamsCB = 3, // b21
	AlbedoSRV = 4, // t0 PS
};

/// ============================
/// Lighting (Directional)
/// ============================
enum class Root_DirectionalLight : uint32_t {
	CameraCB = 0, // b0 PS
	DirLightCB = 1, // b30 PS
	GBufferTable = 2, // t10..t13
};

/// ============================
/// Debug Fullscreen
/// ============================
enum class Root_DebugFullScreen : uint32_t {
	CameraCB = 0, // b0
	DebugCB = 1, // b50
	GBufferTable = 2, // t10..t13
};

/// ============================
/// ShadowCaster (Depth Only)
/// ============================
enum class Root_ShadowCaster : uint32_t {
	ShadowCB = 0, // b40 VS
	ObjectCB = 1, // b10 VS
};

} 
