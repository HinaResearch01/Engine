#ifndef TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI
#define TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI

#include "../Core/Common.hlsli"

uint SelectCascade(float viewDepth, float4 cascadeSplits)
{
    if (viewDepth <= cascadeSplits.x)
        return 0;
    if (viewDepth <= cascadeSplits.y)
        return 1;
    if (viewDepth <= cascadeSplits.z)
        return 2;
    return 3;
}

bool WorldToShadowUVZ(
    float4x4 lightViewProj,
    float3 worldPos,
    out float2 uv,
    out float z)
{
    float4 sp = mul(float4(worldPos, 1.0f), lightViewProj);

    if (abs(sp.w) < 1e-6f)
    {
        uv = 0;
        z = 0;
        return false;
    }

    // NDC
    sp.xyz /= sp.w;

    // ============================
    // Y反転
    // ============================
    uv.x = sp.x * 0.5f + 0.5f;
    uv.y = -sp.y * 0.5f + 0.5f;

    // ============================
    // zを0〜1へ (DX12 Projection is already 0..1)
    // ============================
    z = sp.z;

    // ============================
    // 範囲チェック
    // ============================
    if (uv.x < 0 || uv.x > 1 ||
        uv.y < 0 || uv.y > 1 ||
        z < 0 || z > 1)
        return false;

    return true;
}

float SampleShadowPCF3x3(
    Texture2DArray shadowMap,
    SamplerState samplerState,
    uint cascadeIdx,
    float2 uv,
    float z,
    float2 texelSize)
{
    float sum = 0.0f;

    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 offset = float2(x, y) * texelSize;

            float depth =
                shadowMap.Sample(
                    samplerState,
                    float3(uv + offset, cascadeIdx)).r;

            sum += (z <= depth) ? 1.0f : 0.0f;
        }
    }

    return sum * (1.0f / 9.0f);
}

float ComputeCSMShadowFactor(
    Texture2DArray shadowMap,
    SamplerState samplerState,

    float3 worldPos,
    float3 normalWS,
    float viewDepth,

    float4 cascadeSplits,
    float4x4 lightViewProj[4],

    float2 shadowTexelSize,
    float shadowBias,
    float shadowNormalBias,
    float3 lightDirWS)
{
    uint cascadeIdx = SelectCascade(viewDepth, cascadeSplits);

    float2 uv;
    float z;

    if (!WorldToShadowUVZ(
            lightViewProj[cascadeIdx],
            worldPos,
            uv,
            z))
        return 1.0f;

    float3 L = SafeNormalize(-lightDirWS);

    float ndotl = saturate(dot(SafeNormalize(normalWS), L));
    float normalBias = (1.0f - ndotl) * shadowNormalBias;

    z -= (shadowBias + normalBias);

    return SampleShadowPCF3x3(
        shadowMap,
        samplerState,
        cascadeIdx,
        uv,
        z,
        shadowTexelSize);
}

#endif
