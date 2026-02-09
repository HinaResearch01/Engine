#define TSUMI_DECLARE_CSM_SHADOWMAP

#include "../Interop/Bindings.hlsli"
#include "../Interop/CameraInfo.hlsli"
#include "../Interop/LightInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Math.hlsli"
#include "../Core/Samplers.hlsli"
#include "../Lighting/CSMShadowSampling.hlsli"
#include "../Lighting/SimpleDirectional.hlsli"

// ================================
// GBuffer SRVs
// ================================
Texture2D gGBuffer0_Albedo : register(BIND_GBUFFER_ALBEDO);
Texture2D gGBuffer1_NormalWS : register(BIND_GBUFFER_NORMAL_WS);
Texture2D gGBuffer2_Reflectivity : register(BIND_GBUFFER_REFLECTIVITY);
Texture2D gDepth01 : register(BIND_GBUFFER_DEPTH);

// ================================
// CSM ShadowMap (Texture2DArray)
// ================================
Texture2DArray gShadowMapCSM : register(BIND_SHADOW_TEX);

// ================================
// Fullscreen VS output
// ================================
struct VSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ================================
// Vertex Shader
// ================================
VSOut DirLightingVS(uint vertexID : SV_VertexID)
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
float4 DirLightingPS(VSOut i) : SV_Target
{
    float2 uv = i.uv;

    // ---- GBuffer sampling ----
    float3 albedo =
        gGBuffer0_Albedo.Sample(gPointClamp, uv).rgb;

    float3 N =
        SafeNormalize(
            gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz
        );

    float reflectivity =
        gGBuffer2_Reflectivity.Sample(gPointClamp, uv).r;

    float depth01 =
        gDepth01.Sample(gPointClamp, uv).r;

    // ---- World position ----
    float3 worldPos =
        ReconstructWorldPos(depth01, uv, gInvViewProj);

    // ---- View depth (for cascade selection) ----
    float3 viewPos =
        mul(float4(worldPos, 1.0f), gView).xyz;

    float viewDepth = abs(viewPos.z);

    // ---- View / Light vectors ----
    float3 cameraPosWS = gInvView[3].xyz;
    float3 V = SafeNormalize(cameraPosWS - worldPos);
    float3 L = SafeNormalize(-gLightDirWS);

    // ---- CSM shadow ----
    float shadowFactor =
        ComputeCSMShadowFactor(worldPos, N, viewDepth);

    // ---- Lighting ----
    float3 lit =
        EvalDirectionalLight(
            albedo,
            N,
            V,
            L,
            gRadiance,
            reflectivity
        );

    return float4(lit * shadowFactor, 1.0f);
}
