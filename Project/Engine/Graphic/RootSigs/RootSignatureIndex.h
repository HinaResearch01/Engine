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
	MaterialCB = 2, // b2 PS
	AlbedoSRV = 3, // t0
};

/// ============================
/// GBuffer
/// ============================
enum class Root_GBuffer : uint32_t {
	CameraCB = 0, // b0 VS
	ObjectCB = 1, // b1 VS
	MaterialCB = 2, // b2 PS
	AlbedoSRV = 3, // t0
};

/// ============================
/// Lighting (Directional)
/// ============================
enum class Root_LightingDirectional : uint32_t {
	CameraCB = 0, // b0 PS
	DirectionalCB = 1, // b3 PS
	GBufferTable = 2, // t10..t13
};

/// ============================
/// Debug Fullscreen
/// ============================
enum class Root_DebugFullScreen : uint32_t {
	CameraCB = 0, // b0
	DirectionalCB = 1, // b3
	DebugCB = 2, // b4
	GBufferTable = 3, // t10..t13
};

} 
