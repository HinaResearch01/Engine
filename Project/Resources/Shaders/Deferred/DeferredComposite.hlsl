#include "../Interop/Bindings.hlsli"
#include "../Interop/PostProcessInfo.hlsli"
#include "../Core/Common.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Samplers.hlsli"

// Lighting result
Texture2D gLightingTex : register(BIND_LIGHTING_HDR_TEX);

struct VSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// simple tonemap
float3 TonemapReinhard(float3 x)
{
    return x / (1.0 + x);
}

// tiny ACES-like (軽量近似)
float3 TonemapACES(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 ApplyTonemap(float3 hdr)
{
    float3 x = hdr * max(gExposure, 0.0001);

    if (gTonemapMode == 1)
        x = TonemapReinhard(x);
    else if (gTonemapMode == 2)
        x = TonemapACES(x);
    // 0 = none

    // gamma
    float invGamma = 1.0 / max(gGamma, 0.0001);
    return pow(saturate(x), invGamma);
}

// ================================
// Vertex Shader
// ================================
VSOut CompositeVS(uint vertexID : SV_VertexID)
{
    FullscreenVSOut f = FullscreenVS(vertexID);
    VSOut o;
    o.positionCS = f.positionCS;
    o.uv = f.uv;
    return o;
}

// ================================
// Pixel Shader
// ================================
float4 CompositePS(VSOut i) : SV_Target0
{
    float3 hdr = gLightingTex.Sample(gLinearClamp, i.uv).rgb;
    float3 ldr = ApplyTonemap(hdr);
    return float4(ldr, 1.0);
}