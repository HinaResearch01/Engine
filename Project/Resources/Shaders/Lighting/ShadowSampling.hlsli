#ifndef __LIGHTING_SHADOW_SAMPLING_HLSLI__
#define __LIGHTING_SHADOW_SAMPLING_HLSLI__

#include "../Core/Common.hlsli"
#include "../Core/Samplers.hlsli"
#include "../Interop/ShadowInfo.hlsli"
#include "../Interop/LightInfo.hlsli"

// 呼び出し側で宣言されている前提
Texture2D gShadowMap;

// World -> Shadow UVZ
bool WorldToShadowUVZ(float3 worldPos, out float2 uv, out float z)
{
    float4 sp = mul(float4(worldPos, 1.0f), gLightViewProj);
    if (abs(sp.w) < 1e-6f)
    {
        uv = 0;
        z = 0;
        return false;
    }

    sp.xyz /= sp.w;
    uv = sp.xy * 0.5f + 0.5f;
    z = sp.z;

    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        return false;

    return true;
}

// Single test
float ShadowTest(float2 uv, float z)
{
    float depth = gShadowMap.Sample(gPointClamp, uv).r;
    return (z <= depth) ? 1.0f : 0.0f;
}

// 3x3 PCF
float SampleShadowPCF3x3(float2 uv, float z)
{
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 o = float2(x, y) * gShadowTexelSize;
            sum += ShadowTest(uv + o, z);
        }
    }
    return sum * (1.0f / 9.0f);
}

// ================================
// Normal-bias aware shadow factor
// ================================
float ComputeShadowFactor(float3 worldPos, float3 normalWS)
{
    float2 uv;
    float z;
    if (!WorldToShadowUVZ(worldPos, uv, z))
        return 1.0f;

    // Light direction (to light)
    float3 L = SafeNormalize(-gLightDirWS);

    // N·L dependent normal bias
    // 斜め当たりほど bias を増やす
    float ndotl = saturate(dot(SafeNormalize(normalWS), L));
    float normalBias = (1.0f - ndotl) * gShadowNormalBias;

    // Apply biases
    z -= (gShadowBias + normalBias);

    return SampleShadowPCF3x3(uv, z);
}


#endif // __LIGHTING_SHADOW_SAMPLING_HLSLI__
