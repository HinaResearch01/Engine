#ifndef __TRANSFORM_INFO_HLSLI__
#define __TRANSFORM_INFO_HLSLI__

// Transform (per-draw) constant buffer
struct TransformCB
{
    row_major float4x4 gWorld; // Local -> World
    row_major float4x4 gWorldInvTranspose; // For normal transform
};

#endif // __TRANSFORM_INFO_HLSLI__
