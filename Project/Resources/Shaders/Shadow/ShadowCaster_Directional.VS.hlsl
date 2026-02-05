#include "../Common/Transform.hlsli"
#include "../Common/Light.hlsli"

struct VS_INPUT
{
    float3 position : POSITION;
};
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
};

ConstantBuffer<ShadowCasterLightMatricesCB> gShadow : register(b0);
ConstantBuffer<ShadowCasterTransformCB> gTransform : register(b1);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT o;
    float4 posWS = mul(float4(input.position, 1.0f), gTransform.world);
    o.position = mul(posWS, gShadow.lightViewProj);
    return o;
}