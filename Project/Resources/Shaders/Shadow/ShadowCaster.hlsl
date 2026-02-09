#include "../Interop/Bindings.hlsli"
#include "../Interop/TransformInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

struct VSIn
{
    float3 position : POSITION;
};
struct VSOut
{
    float4 positionCS : SV_POSITION;
};

// ================================
// Vertex Shader
// ================================
VSOut ShadowVS(VSIn v)
{
    VSOut o;
    float4 wpos = mul(float4(v.position, 1.0f), gWorld);
    o.positionCS = mul(wpos, gLightViewProj);
    return o;
}

// ================================
// Pixel Shader : Depth-only PSは不要
// ================================
float4 ShadowPS(VSOut i) : SV_Target0
{
    return 0;
}
