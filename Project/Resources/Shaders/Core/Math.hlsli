#ifndef _MATH_HLSLI__
#define _MATH_HLSLI__

#include "Common.hlsli"

// Reconstruct world position from depth (0..1) and UV using inverse view-projection
float3 ReconstructWorldPos(float depth01, float2 uv, float4x4 invViewProj)
{
    float2 ndcXY = float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
    float4 clip = float4(ndcXY, depth01, 1.0f);
    float4 wpos = mul(clip, invViewProj);
    wpos.xyz /= max(wpos.w, 1e-6f);
    return wpos.xyz;
}

#endif // __MATH_HLSLI__