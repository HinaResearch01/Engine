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

    // ---------------- GBuffer sampling ----------------
    float3 albedo =
        gGBuffer0_Albedo.Sample(gPointClamp, uv).rgb;
    float3 N =
        SafeNormalize(
            gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz);
    float reflectivity =
        gGBuffer2_Reflectivity.Sample(gPointClamp, uv).r;
    float depth01 =
        gDepth01.Sample(gPointClamp, uv).r;

    // ---------------- World position reconstruction ----------------
    float3 worldPos =
        ReconstructWorldPos(
            depth01,
            uv,
            gCamera.gInvViewProj);
    float3 viewPos =
        mul(float4(worldPos, 1.0f), gCamera.gView).xyz;
    float viewDepth = abs(viewPos.z);

    // ---------------- View / Light vectors ----------------
    float3 cameraPosWS = gCamera.gInvView[3].xyz;
    float3 V = SafeNormalize(cameraPosWS - worldPos);
    float3 L = SafeNormalize(-gLight.gLightDirWS);

    // ---------------- CSM shadow ----------------
    float shadowFactor =
        ComputeCSMShadowFactor(
            gShadowMapCSM,
            gPointClamp,
            worldPos,
            N,
            viewDepth,
            gShadow.gCascadeSplitDepths,
            gShadow.gLightViewProj,
            gShadow.gShadowTexelSize,
            gShadow.gShadowBias,
            gShadow.gShadowNormalBias,
            gLight.gLightDirWS
        );

    // ---------------- Lighting ----------------
    float3 lit =
        EvalDirectionalLight(
            albedo,
            N,
            V,
            L,
            gLight.gRadiance,
            reflectivity
        );

    return float4(lit * shadowFactor, 1.0f);
}
