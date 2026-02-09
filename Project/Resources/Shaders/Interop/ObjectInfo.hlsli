#ifndef __OBJECT_INFO_HLSLI__
#define __OBJECT_INFO_HLSLI__

// Object (per-draw) constant buffer
cbuffer ObjectCB : register(b10)
{
    float4x4 gWorld; // Local -> World
    float4x4 gWorldInvTranspose; // For normal transform
};

#endif // __OBJECT_INFO_HLSLI__
