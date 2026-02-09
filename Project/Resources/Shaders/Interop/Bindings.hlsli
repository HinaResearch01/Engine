#ifndef TSUMI_INTEROP_BINDINGS_HLSLI
#define TSUMI_INTEROP_BINDINGS_HLSLI

// =======================================================
// Constant Buffers (Global Slot Convention)
// =======================================================
#define BIND_CAMERA_CB               b0
#define BIND_TRANSFORM_CB            b10
#define BIND_MATERIAL_CB             b11
#define BIND_DIRECTIONAL_LIGHT_CB    b20
#define BIND_SHADOW_CB               b21
#define BIND_POST_CB                 b30
#define BIND_DEFERRED_DEBUG_CB       b50

// =======================================================
// SRV TABLE : MATERIAL (GBuffer Pass)
// RootParameter = MaterialSRVTable
// =======================================================
#define BIND_MAT_TEX_ALBEDO           t0
// t1.. reserved for normal / orm etc.

// =======================================================
// SRV TABLE : GBUFFER (Deferred / Debug Pass)
// RootParameter = GBufferSRVTable
// =======================================================
#define BIND_GBUFFER_ALBEDO           t0
#define BIND_GBUFFER_NORMAL_WS        t1
#define BIND_GBUFFER_REFLECTIVITY     t2
#define BIND_GBUFFER_DEPTH            t3

// =======================================================
// SRV TABLE : SHADOW (Deferred Lighting)
// RootParameter = ShadowSRVTable
// =======================================================
// NOTE:
// - non-CSM  : Texture2D
// - CSM      : Texture2DArray
#define BIND_SHADOW_TEX               t0

// =======================================================
// SRV TABLE : POST / COMPOSITE
// RootParameter = PostSRVTable
// =======================================================
#define BIND_POST_LIGHTING_HDR        t0

// =======================================================
// Samplers (Static or Sampler Table)
// =======================================================
#define BIND_POINT_CLAMP              s0
#define BIND_LINEAR_WRAP              s1
#define BIND_LINEAR_CLAMP             s2

#endif // TSUMI_INTEROP_BINDINGS_HLSLI
