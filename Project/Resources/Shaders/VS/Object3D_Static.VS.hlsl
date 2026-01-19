#include "../Common/Transform.hlsli"
#include "../Common/Camera.hlsli"

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

ConstantBuffer<CameraMatricesCB> gCameraMat : register(b0);
ConstantBuffer<TransformCB> gTransform : register(b1);

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = mul(gTransform.world, float4(input.position, 1.0f));
    output.position = mul(gCameraMat.viewProj, worldPos);
    output.texCoord = input.texCoord;

    return output;
}