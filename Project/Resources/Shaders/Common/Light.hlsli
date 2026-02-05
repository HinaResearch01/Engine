#ifndef LIGHT_HLSLI
#define LIGHT_HLSLI

struct DirectionalLightMatricesCB
{
    row_major float4x4 view;
    row_major float4x4 proj;
    row_major float4x4 viewProj;
    float3 position;
};

struct ShadowCasterLightMatricesCB
{
    row_major float4x4 lightViewProj;
};

#endif