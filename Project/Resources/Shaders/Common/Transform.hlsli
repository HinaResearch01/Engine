#ifndef TRANSFORM_HLSLI
#define TRANSFORM_HLSLI

struct TransformCB
{
    row_major float4x4 world;
    row_major float4x4 worldInverseTranspose;
};

struct ShadowCasterTransformCB
{
    row_major float4x4 world; // b1
};

#endif