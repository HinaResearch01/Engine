#include "../Common/Material.hlsli"

struct PS_INPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

ConstantBuffer<MaterialCB> gMaterial : register(b1);

Texture2D gAlbedoTexture : register(t0);
SamplerState gLinearSampler : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
    float2 uv =
        mul(float4(input.texCoord, 0.0f, 1.0f), gMaterial.uvTransform).xy;

    float4 texColor = gAlbedoTexture.Sample(gLinearSampler, uv);

    return texColor * gMaterial.color;
}