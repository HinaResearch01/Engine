#ifndef TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI
#define TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI

#include "../Core/Common.hlsli"

// ポアソンディスクのオフセット定義 (16サンプル)
static const float2 POISSON_DISK[16] =
{
    float2(-0.94201624, -0.39906216), float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870), float2(0.34495938, 0.29387760),
    float2(-0.91588401, 0.45771432), float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845), float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554), float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023), float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507), float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367), float2(0.14383161, -0.14100790)
};

float SampleShadowPoisson(
    Texture2DArray shadowMap,
    SamplerState samplerState,
    uint cascadeIdx,
    float2 uv,
    float z,
    float2 texelSize)
{
    float sum = 0.0f;
    
    float filterRadius = 2.0f;

    [unroll]
    for (int i = 0; i < 16; ++i)
    {
        float2 offset = POISSON_DISK[i] * texelSize * filterRadius;
        float depth = shadowMap.SampleLevel(samplerState, float3(uv + offset, cascadeIdx), 0).r;
        sum += (z <= depth) ? 1.0f : 0.0f;
    }

    return sum / 16.0f;
}

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
    // 💡【修正】ゴミを拾わないように 0.0f と 1.0f で厳格に弾く
    // ============================
    if (uv.x < 0.0f || uv.x > 1.0f ||
        uv.y < 0.0f || uv.y > 1.0f ||
        z < 0.0f || z > 1.0f)
    {
        return false;
    }

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
            float depth = shadowMap.SampleLevel(samplerState, float3(uv + offset, cascadeIdx), 0).r;
            sum += (z <= depth) ? 1.0f : 0.0f;
        }
    }

    return sum * (1.0f / 9.0f);
}

float SampleShadowPCF5x5(
    Texture2DArray shadowMap,
    SamplerState samplerState,
    uint cascadeIdx,
    float2 uv,
    float z,
    float2 texelSize)
{
    float sum = 0.0f;

    [unroll]
    for (int y = -2; y <= 2; ++y)
    {
        [unroll]
        for (int x = -2; x <= 2; ++x)
        {
            float2 offset = float2(x, y) * texelSize;
            float depth = shadowMap.SampleLevel(samplerState, float3(uv + offset, cascadeIdx), 0).r;
            sum += (z <= depth) ? 1.0f : 0.0f;
        }
    }

    return sum * (1.0f / 25.0f);
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

    // 1. Apply Normal Bias to prevent shadow acne
    float cascadeScale = shadowTexelSize.x * 1000.0f;
    float3 biasedPos = worldPos + normalWS * (shadowNormalBias * cascadeScale);

    // 2. 該当カスケードの行列でUVとZを計算
    bool isInside = WorldToShadowUVZ(lightViewProj[cascadeIdx], biasedPos, uv, z);

    // 境界付近でUVがギリギリ範囲外だった場合、1つ奥のカスケードを試す
    if (!isInside && cascadeIdx < 3)
    {
        cascadeIdx++;
        isInside = WorldToShadowUVZ(lightViewProj[cascadeIdx], biasedPos, uv, z);
    }

    // 3. それでも範囲外なら影なし（ライトが当たる）
    if (!isInside)
    {
        return 1.0f;
    }
    
    // 4. 深度バイアスの適用
    z -= shadowBias;

    // 5. PCFサンプリング
    return SampleShadowPoisson(shadowMap, samplerState, cascadeIdx, uv, z, shadowTexelSize);
}

#endif
