#pragma once
#include <cstdint>

namespace Tsumi::Graphic::RootIndex {

template<class TRootEnum>
constexpr uint32_t ToRoot(TRootEnum e) {
	return static_cast<uint32_t>(e);
}

/// ============================
/// GBuffer
/// ============================
enum class Root_GBuffer : uint32_t {
	CameraCB = 0,          // b0
	TransformCB = 1,       // b1
	MaterialParamsCB = 2,  // b2
	AlbedoSRV = 3          // t0
};

/// ============================
/// Lighting (Directional)
/// ============================
enum class Root_DirectionalLight : uint32_t {
	CameraCB = 0,      // b0
	DirLightCB = 1,    // b1
	ShadowCB = 2,      // b2
	GBufferTable = 3   // t0..t4
};

/// ============================
/// Lighting (Point)
/// ============================
enum class Root_PointLight : uint32_t {
	CameraCB = 0,      // b0
	PointLightCB = 1,  // b1
	GBufferTable = 2   // t0..t3
};

/// ============================
/// Lighting (Spot)
/// ============================
enum class Root_SpotLight : uint32_t {
	CameraCB = 0,      // b0
	SpotLightCB = 1,   // b1
	GBufferTable = 2   // t0..t3
};

/// ============================
/// Deferred Debug
/// ============================
enum class Root_DeferredDebug : uint32_t {
	DebugCB = 0,       // b0
	GBufferTable = 1   // t0..t3
};

/// ============================
/// Deferred Composite
/// ============================
enum class Root_DeferredComposite : uint32_t {
	PostCB = 0,        // b0
	LightingSRV = 1    // t0
};

/// ============================
/// ShadowCaster (Depth Only)
/// ============================
enum class Root_ShadowCaster : uint32_t {
	TransformCB = 0,   // b0
	ShadowCB = 1,      // b1
	CascadeIndexCB = 2 // b2
};

} // namespace Tsumi::Graphic::RootIndex
