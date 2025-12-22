#ifndef MATERIAL_HLSLI
#define MATERIAL_HLSLI

struct MaterialCB
{
    float4 color;
    float4x4 uvTransform;
};

#endif