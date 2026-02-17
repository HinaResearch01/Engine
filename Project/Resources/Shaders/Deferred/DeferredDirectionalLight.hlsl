#include "../Interop/CameraInfo.hlsli"
#include "../Interop/LightInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

#include "../Core/Common.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Math.hlsli"

#include "../Lighting/CSMShadowSampling.hlsli"
#include "../Lighting/PBR.hlsli"

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

    // Float format stores -1..1 directly
    float3 normal = gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz; 
    normal = normalize(normal);

    float4 materialData = gGBuffer2_Reflectivity.Sample(gPointClamp, uv);
    float metallic = materialData.r;
    float roughness = materialData.g;
    float ao = materialData.b;

    float depth = gDepth01.Sample(gPointClamp, uv).r;

    // 2. Reconstruct World Position
    float3 positionWS = ReconstructWorldPos(depth, uv, gCamera.gInvViewProj);

    /// 3. Shadow Calculation
    float viewDepth = mul(float4(positionWS, 1.0f), gCamera.gView).z;
    
    float shadow = ComputeCSMShadowFactor(
        gShadowMapCSM,
        gPointClamp,
        positionWS,
        normal,
        viewDepth,
        gShadow.gCascadeSplitDepths,
        gShadow.gLightViewProj,
        gShadow.gShadowTexelSize,
        gShadow.gShadowBias,
        gShadow.gShadowNormalBias,
        gLight.gLightDirWS
    );

    // 4. Lighting Calculation
    float3 V = normalize(gCamera.gCameraPosWS - positionWS);
    float3 L = normalize(-gLight.gLightDirWS);
    
    float3 lighting = LightingPBR(
        positionWS,
        normal,
        V,
        L,
        albedo,
        metallic,
        roughness,
        gLight.gRadiance * gLight.gIntensity
    );

    // Ambient
    float3 ambient = albedo * gLight.gAmbientColor * ao;
    
    // Shadow application
    float3 finalColor = ambient + lighting * shadow;
    
    return float4(finalColor, 1.0f);
}
