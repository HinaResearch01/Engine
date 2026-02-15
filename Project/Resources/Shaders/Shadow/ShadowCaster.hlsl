#include "../Interop/TransformInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

// ============================================================
// Constant Buffers
// ============================================================
// per-draw transform
ConstantBuffer<TransformCB> gTransform : register(b0);
// shadow data (CSM)
ConstantBuffer<ShadowCB> gShadow : register(b1);
// cascade index
cbuffer ShadowCascadeCB : register(b2)
{
    uint gCascadeIndex;
    float3 _pad;
}

// ============================================================
// Input / Output
// ============================================================
struct VSIn
{
    float3 position : POSITION;
};
struct VSOut
{
    float4 positionCS : SV_POSITION;
};

// ============================================================
// Vertex Shader
// ============================================================
VSOut ShadowVS(VSIn v)
{
    VSOut o;

    float4 wpos =
        mul(float4(v.position, 1.0f), gTransform.gWorld);
    float4x4 lightVP =
        gShadow.gLightViewProj[gCascadeIndex];
    o.positionCS = mul(wpos, lightVP);

    return o;
}

// ============================================================
// Pixel Shader (Depth Only)
// ============================================================
float4 ShadowPS(VSOut i) : SV_Target0
{
    return 0;
}
