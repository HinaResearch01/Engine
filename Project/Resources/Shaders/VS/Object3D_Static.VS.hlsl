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
    VS_OUTPUT o;
    float4 pos = float4(input.position, 1.0f);

    // 行列が Row-Major の場合、ベクトルを左に置く
    pos = mul(pos, gTransform.world);
    pos = mul(pos, gCameraMat.view);
    pos = mul(pos, gCameraMat.proj);

    o.position = pos;
    o.texCoord = input.texCoord;
    return o;
}