#ifndef TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI
#define TSUMI_LIGHTING_CSM_SHADOW_SAMPLING_HLSLI

#include "../Core/Common.hlsli"
#include "../Core/Samplers.hlsli"
#include "../Interop/ShadowInfo.hlsli"
#include "../Interop/LightInfo.hlsli"

#ifndef TSUMI_DECLARE_CSM_SHADOWMAP
Texture2DArray gShadowMapCSM;
#endif

// ---- cascade selection (view space depth) ----
// viewDepth = distance along camera forward in view space (positive)
uint SelectCascade(float viewDepth)
{
    // splits are far distances
    if (viewDepth <= gCascadeSplitDepths.x)
        return 0;
    if (viewDepth <= gCascadeSplitDepths.y)
        return 1;
    if (viewDepth <= gCascadeSplitDepths.z)
        return 2;
    return 3;
}

bool WorldToShadowUVZ(uint cascadeIdx, float3 worldPos, out float2 uv, out float z)
{
    float4 sp = mul(float4(worldPos, 1.0f), gLightViewProj[cascadeIdx]);
    if (abs(sp.w) < 1e-6f)
    {
        uv = 0;
        z = 0;
        return false;
    }

    sp.xyz /= sp.w;

    uv = sp.xy * 0.5f + 0.5f;
    z = sp.z;

    // outside cascade
    if (uv.x < 0 || uv.x > 1 || uv.y < 0 || uv.y > 1)
        return false;

    return true;
}

float ShadowTest(uint cascadeIdx, float2 uv, float z)
{
    float depth = gShadowMapCSM.Sample(gPointClamp, float3(uv, cascadeIdx)).r;
    return (z <= depth) ? 1.0f : 0.0f;
}

float SampleShadowPCF3x3(uint cascadeIdx, float2 uv, float z)
{
    float sum = 0.0f;
    [unroll]
    for (int y = -1; y <= 1; ++y)
    {
        [unroll]
        for (int x = -1; x <= 1; ++x)
        {
            float2 o = float2(x, y) * gShadowTexelSize;
            sum += ShadowTest(cascadeIdx, uv + o, z);
        }
    }
    return sum * (1.0f / 9.0f);
}

// Main
float ComputeCSMShadowFactor(float3 worldPos, float3 normalWS, float viewDepth)
{
    uint cascadeIdx = SelectCascade(viewDepth);

    float2 uv;
    float z;
    if (!WorldToShadowUVZ(cascadeIdx, worldPos, uv, z))
        return 1.0f;

    float3 L = SafeNormalize(-gLightDirWS);

    float ndotl = saturate(dot(SafeNormalize(normalWS), L));
    float normalBias = (1.0f - ndotl) * gShadowNormalBias;

    z -= (gShadowBias + normalBias);

    return SampleShadowPCF3x3(cascadeIdx, uv, z);
}

#endif