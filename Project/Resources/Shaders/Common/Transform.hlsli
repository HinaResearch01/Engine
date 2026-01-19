#ifndef TRANSFORM_HLSLI
#define TRANSFORM_HLSLI

struct TransformCB
{
    row_major float4x4 world;
    row_major float4x4 worldInverseTranspose;
};

#endif