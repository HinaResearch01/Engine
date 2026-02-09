#ifndef _MATH_HLSLI__
#define _MATH_HLSLI__

#include "Common.hlsli"

// Reconstruct world position from depth (0..1) and UV using inverse view-projection
float3 ReconstructWorldPos(float depth01, float2 uv, float4x4 invViewProj)
{
    // NDC
    float2 ndcXY = uv * 2.0f - 1.0f;
    float ndcZ = depth01 * 2.0f - 1.0f;

    float4 clip = float4(ndcXY, ndcZ, 1.0f);
    float4 wpos = mul(clip, invViewProj);
    wpos.xyz /= max(wpos.w, 1e-6f);
    return wpos.xyz;
}

#endif // __MATH_HLSLI__