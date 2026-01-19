#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI

struct MaterialCB
{
    float4 color;
    row_major float3x3 uvTransform;
};

#endif