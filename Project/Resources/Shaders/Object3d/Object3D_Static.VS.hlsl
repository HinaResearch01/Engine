#include "Object3D_Common.hlsli"

struct TransformCB
{
    float4x4 world;
    float4x4 viewProj;
};

ConstantBuffer<TransformCB> gTransform : register(b0);

VSOutput main(VSInput input)
{
    VSOutput output;

    float4 worldPos = mul(float4(input.position, 1.0f), gTransform.world);
    output.position = mul(worldPos, gTransform.viewProj);
    output.texCoord = input.texCoord;

    return output;
}