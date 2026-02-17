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
ConstantBuffer<SpotLightCB> gLight : register(b1);

// ============================================================
// GBuffer SRVs
// ============================================================
Texture2D gGBuffer0_Albedo : register(t0);
Texture2D gGBuffer1_NormalWS : register(t1);
Texture2D gGBuffer2_Reflectivity : register(t2);
Texture2D gDepth01 : register(t3);

// t4 is unused (or reserved for CSM in other shaders)
Texture2DArray gSpotShadowMap : register(t5);

// Sampler
SamplerState gPointClamp : register(s0);
SamplerComparisonState gShadowSampler : register(s1);

// ============================================================
// Fullscreen VS
// ============================================================
struct VSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut SpotLightingVS(uint vertexID : SV_VertexID)
{
    FullscreenVSOut f = FullscreenVS(vertexID);

    VSOut o;
    o.positionCS = f.positionCS;
    o.uv = f.uv;
    return o;
}

// ============================================================
// Spot Shadow Calculation
// ============================================================
float CalcSpotShadow(float3 positionWS)
{
	if (gLight.gShadowIndex < 0) return 1.0f;

	float4 posLS = mul(float4(positionWS, 1.0f), gLight.gLightViewProj);
	float3 projCoords = posLS.xyz / posLS.w;

	projCoords.x = projCoords.x * 0.5f + 0.5f;
	projCoords.y = -projCoords.y * 0.5f + 0.5f;

	if (projCoords.z > 1.0f || projCoords.z < 0.0f ||
		projCoords.x < 0.0f || projCoords.x > 1.0f ||
		projCoords.y < 0.0f || projCoords.y > 1.0f)
	{
		return 1.0f; // Outside light frustum
	}

	// Bias
	float currentDepth = projCoords.z;
	float bias = gLight.gShadowBias;

	// Sample
	// float3(uv.x, uv.y, slice)
	float shadow = gSpotShadowMap.SampleCmpLevelZero(
		gShadowSampler,
		float3(projCoords.xy, (float)gLight.gShadowIndex),
		currentDepth - bias
	).r;

	return shadow;
}

// ============================================================
// Pixel Shader
// ============================================================
float4 SpotLightingPS(VSOut i) : SV_Target
{
    float2 uv = i.uv;

    // 1. GBuffer Sampling
    float4 albedoData = gGBuffer0_Albedo.Sample(gPointClamp, uv);
    if (albedoData.a == 0.0f) discard;

    float3 albedo = albedoData.rgb;
    float3 normal = normalize(gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz);

    float4 materialData = gGBuffer2_Reflectivity.Sample(gPointClamp, uv);
    float metallic = materialData.r;
    float roughness = materialData.g;

    float depth = gDepth01.Sample(gPointClamp, uv).r;

    // 2. Reconstruct World Position
    float3 positionWS = ReconstructWorldPos(depth, uv, gCamera.gInvViewProj);

    // 3. Lighting Calculation
    float3 lightVec = gLight.gPositionWS - positionWS;
    float distance = length(lightVec);
    
    // Range check
    if (distance > gLight.gRange) discard;

    float3 L = normalize(lightVec);
    float3 V = normalize(gCamera.gCameraPosWS - positionWS);

    // Attenuation (Distance)
    float d = distance / gLight.gRange;
    float attenuation = saturate(1.0f - d * d);
    attenuation *= attenuation;

    // Spot Factor (Cone)
    float cosAngle = dot(-L, normalize(gLight.gDirectionWS));
    
    // innerCos > outerCos (e.g. 0.9 > 0.8)
    float spotFactor = saturate((cosAngle - gLight.gOuterCos) / (gLight.gInnerCos - gLight.gOuterCos));
    
    attenuation *= spotFactor;

    if (attenuation <= 0.0f) discard;

    // Shadow
    float shadow = CalcSpotShadow(positionWS);
    attenuation *= shadow;

    // Radiance
    float3 radiance = gLight.gRadiance * attenuation;

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

    return float4(lighting, 1.0f);
}
