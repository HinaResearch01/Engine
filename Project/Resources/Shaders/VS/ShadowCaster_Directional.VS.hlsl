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

ConstantBuffer<DirectionalLightMatricesCB> gLight : register(b0);
ConstantBuffer<TransformCB> gTransform : register(b1);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT o;
    float4 pos = float4(input.position, 1.0f);

    pos = mul(pos, gTransform.world);
    pos = mul(pos, gLight.viewProj);

    o.position = pos;
    
    return o;
}