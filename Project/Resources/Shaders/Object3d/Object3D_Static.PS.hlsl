#include "Object3D_Common.hlsli"

struct MaterialCB
{
    float4 color;
    float4x4 uvTransform;
};

ConstantBuffer<MaterialCB> gMaterial : register(b1);

Texture2D gAlbedoTexture : register(t0);
SamplerState gLinearSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET
{
    float2 uv =
        mul(float4(input.texCoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;

    float4 texColor = gAlbedoTexture.Sample(gLinearSampler, uv);

    return texColor * gMaterial.color;
}