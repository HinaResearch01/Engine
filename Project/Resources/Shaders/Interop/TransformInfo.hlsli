#ifndef __TRANSFORM_INFO_HLSLI__
#define __TRANSFORM_INFO_HLSLI__

// Transform (per-draw) constant buffer
cbuffer TransformCB : register(BIND_TRANSFORM_CB)
{
    float4x4 gWorld; // Local -> World
    float4x4 gWorldInvTranspose; // For normal transform
};

#endif // __TRANSFORM_INFO_HLSLI__
