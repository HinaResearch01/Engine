#include "../Interop/CameraInfo.hlsli"
#include "../Interop/LightInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

#include "../Core/Common.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Math.hlsli"

#include "../Lighting/PBR.hlsli"

// ============================================================
// Constant Buffers
// ============================================================
ConstantBuffer<CameraMatricesCB> gCamera : register(b0);
ConstantBuffer<PointLightCB> gLight : register(b1);
// ShadowCB not used for point lights yet (or use b2 if needed)

// ============================================================
// GBuffer SRVs
// ============================================================
Texture2D gGBuffer0_Albedo : register(t0);
Texture2D gGBuffer1_NormalWS : register(t1);
Texture2D gGBuffer2_Reflectivity : register(t2);
Texture2D gDepth01 : register(t3);

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

VSOut PointLightingVS(uint vertexID : SV_VertexID)
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
float4 PointLightingPS(VSOut i) : SV_Target
{
    float2 uv = i.uv;

    // 1. GBuffer Sampling
    float4 albedoData = gGBuffer0_Albedo.Sample(gPointClamp, uv);
    float3 albedo = albedoData.rgb;
    
    // Check alpha to skip empty pixels (optional, dependent on GBuffer setup)
    if (albedoData.a == 0.0f) discard; 

    float3 normal = gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz; 
    normal = normalize(normal);

    float4 materialData = gGBuffer2_Reflectivity.Sample(gPointClamp, uv);
    float metallic = materialData.r;
    float roughness = materialData.g;
    // float ao = materialData.b; // AO is usually ambient only

    float depth = gDepth01.Sample(gPointClamp, uv).r;

    // 2. Reconstruct World Position
    float3 positionWS = ReconstructWorldPos(depth, uv, gCamera.gInvViewProj);

    // 3. Lighting Calculation
    float3 lightVec = gLight.gPositionWS - positionWS;
    float distance = length(lightVec);
    
    // Attenuation
    if (distance > gLight.gRange) discard;

    float3 L = normalize(lightVec);
    float3 V = normalize(gCamera.gCameraPosWS - positionWS);
    
    // Simple linear attenuation or inverse square
    // Using a smooth falloff based on range
    float d = distance / gLight.gRange;
    float attenuation = saturate(1.0f - d * d); // Simple quadratic falloff
    attenuation *= attenuation; // Quartic falloff

    // Radiance
    float3 radiance = gLight.gRadiance * gLight.gIntensity * attenuation;

    float3 lighting = LightingPBR(
        positionWS,
        normal,
        V,
        L,
        albedo,
        metallic,
        roughness,
        radiance
    );

    // Additive blending assumed in PSO
    return float4(lighting, 1.0f);
}
