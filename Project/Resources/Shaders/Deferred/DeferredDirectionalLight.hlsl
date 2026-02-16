#include "../Interop/CameraInfo.hlsli"
#include "../Interop/LightInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

#include "../Core/Common.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Math.hlsli"

#include "../Lighting/CSMShadowSampling.hlsli"
#include "../Lighting/SimpleDirectional.hlsli"

// ============================================================
// Constant Buffers
// ============================================================
ConstantBuffer<CameraMatricesCB> gCamera : register(b0);
ConstantBuffer<DirectionalLightCB> gLight : register(b1);
ConstantBuffer<ShadowCB> gShadow : register(b2);

// ============================================================
// GBuffer SRVs
// ============================================================
Texture2D gGBuffer0_Albedo : register(t0);
Texture2D gGBuffer1_NormalWS : register(t1);
Texture2D gGBuffer2_Reflectivity : register(t2);
Texture2D gDepth01 : register(t3);

// Shadow map
Texture2DArray gShadowMapCSM : register(t4);

// Sampler
SamplerState gPointClamp : register(s0);

// ============================================================
// Fullscreen VS
// ============================================================
struct VSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut DirLightingVS(uint vertexID : SV_VertexID)
{
    FullscreenVSOut f = FullscreenVS(vertexID);

    VSOut o;
    o.positionCS = f.positionCS;
    o.uv = f.uv;
    return o;
}

// ============================================================
// Pixel Shader
// ============================================================
float4 DirLightingPS(VSOut i) : SV_Target
{
    float2 uv = i.uv;

    // 1. GBuffer Sampling
    float4 albedoData = gGBuffer0_Albedo.Sample(gPointClamp, uv);
    float3 albedo = albedoData.rgb;
    float alpha = albedoData.a;

    float3 normal = gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz; // Float format stores -1..1 directly
    normal = normalize(normal);

    float depth = gDepth01.Sample(gPointClamp, uv).r;

    // 2. Reconstruct World Position
    float3 positionWS = ReconstructWorldPos(depth, uv, gCamera.gInvViewProj);

    // 3. Shadow Calculation
    float shadow = 1.0f;
    float viewDepth = mul(float4(positionWS, 1.0f), gCamera.gView).z;

    // シャドウ計算
    // ※シャドウマップテクスチャがバインドされていない場合は 1.0 が返ることを期待
    shadow = ComputeCSMShadowFactor(
        gShadowMapCSM,
        gPointClamp, // シャドウマップ用サンプラー（PCF用にはLinearが望ましいかもだが一旦Point）
        positionWS,
        normal,
        viewDepth,
        gShadow.gCascadeSplitDepths,
        gShadow.gLightViewProj, // 配列
        gShadow.gShadowTexelSize, // texelSize
        gShadow.gShadowBias, // Shadow Bias
        gShadow.gShadowNormalBias, // Normal Bias
        gLight.gLightDirWS
    );

    // 4. Lighting Calculation
    // Simple Directional Light
    float3 L = normalize(-gLight.gLightDirWS);
    float NdotL = saturate(dot(normal, L));

    // float3 diffuse = albedo * gLight.gRadiance * NdotL * shadow;
    // return float4(diffuse, 1.0f);

    // DEBUG: Visualize Cascade Index
    uint cascadeIdx = SelectCascade(viewDepth, gShadow.gCascadeSplitDepths);
    if (cascadeIdx == 0) return float4(1, 0, 0, 1); // Red
    if (cascadeIdx == 1) return float4(0, 1, 0, 1); // Green
    if (cascadeIdx == 2) return float4(0, 0, 1, 1); // Blue
    return float4(1, 1, 1, 1); // White
}
