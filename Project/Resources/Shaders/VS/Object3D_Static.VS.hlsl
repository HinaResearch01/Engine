#include "../Common/Transform.hlsli"

struct VS_INPUT
{
    float3 position : POSITION;
    float2 texCoord : TEXCOORD0;
};
struct VS_OUTPUT
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

ConstantBuffer<TransformCB> gTransform : register(b0);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = mul(gTransform.world, float4(input.position, 1.0f));
    output.position = mul(gTransform.viewProj, worldPos);
    output.texCoord = input.texCoord;

    return output;
}