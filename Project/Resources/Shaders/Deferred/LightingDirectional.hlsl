// ================================
// LightingDirectional.hlsl
// VS Entry: FullscreenVS
// PS Entry: LightingDirectionalPS
// ================================
#include "../Common/DeferredCommon.hlsli"

// -------------------------------
// Vertex Shader: FullscreenVS
// -------------------------------
VS_OUTPUT_FULLSCREEN LightingDirVS(uint vertexID : SV_VertexID)
{
    return FullscreenVS(vertexID);
}

// -------------------------------
// Pixel Shader: LightingDirectionalPS
// -------------------------------
float4 LightingDirPS(VS_OUTPUT_FULLSCREEN input) : SV_Target0
{
    return float4(1, 0, 1, 1);
}
