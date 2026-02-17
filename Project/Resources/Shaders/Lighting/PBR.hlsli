#ifndef __PBR_HLSLI__
#define __PBR_HLSLI__

#include "../Core/Math.hlsli"

// ===============================================================================================
// Physics Based Rendering (PBR)
// ===============================================================================================

// Constant
static const float PI = 3.14159265359f;

// ------------------------------------------------------------
// Fresnel (Schlick)
// F0: Surface reflection at zero incidence
// ------------------------------------------------------------
float3 F_Schlick(float3 F0, float3 V, float3 H)
{
    float VdotH = saturate(dot(V, H));
    return F0 + (1.0f - F0) * pow(1.0f - VdotH, 5.0f);
}

// ------------------------------------------------------------
// Normal Distribution Function (GGX / Trowbridge-Reitz)
// a: Roughness^2
// ------------------------------------------------------------
float D_GGX(float NdotH, float a)
{
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0f) + 1.0f);
    denom = PI * denom * denom;

    return nom / max(denom, 0.00001f);
}

// ------------------------------------------------------------
// Geometry Function (Smith / Schlick-GGX)
// k: Remapped roughness
// ------------------------------------------------------------
float G_SchlickGGX(float NdotV, float k)
{
    float nom = NdotV;
    float denom = NdotV * (1.0f - k) + k;

    return nom / max(denom, 0.00001f);
}

float G_Smith(float NdotV, float NdotL, float roughness)
{
    // Direct lighting remapping
    float r = (roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float ggx1 = G_SchlickGGX(NdotV, k);
    float ggx2 = G_SchlickGGX(NdotL, k);

    return ggx1 * ggx2;
}

// ------------------------------------------------------------
// Main PBR Calculation
// ------------------------------------------------------------
float3 LightingPBR(
    float3 positionWS,
    float3 normalWS,
    float3 V,           // View Direction (Camera - Pos)
    float3 L,           // Light Direction (e.g. -LightDir)
    float3 albedo,
    float metallic,
    float roughness,
    float3 radiance)    // Light Color * Intensity
{
    float3 N = normalize(normalWS);
    float3 H = normalize(V + L);

    float NdotV = abs(dot(N, V)) + 1e-5f; // avoid artifact
    float NdotL = saturate(dot(N, L));
    float NdotH = saturate(dot(N, H));

    // Dialectics (0.04) vs Metal (Albedo)
    float3 F0 = float3(0.04f, 0.04f, 0.04f); 
    F0 = lerp(F0, albedo, metallic);

    // Cook-Torrance BRDF
    float  D = D_GGX(NdotH, roughness * roughness);
    float  G = G_Smith(NdotV, NdotL, roughness);
    float3 F = F_Schlick(F0, V, H);

    float3 numerator = D * G * F;
    float denominator = 4.0f * NdotV * NdotL + 0.0001f;
    float3 specular = numerator / denominator;

    // Energy Conservation
    float3 kS = F;
    float3 kD = float3(1.0f, 1.0f, 1.0f) - kS;
    kD *= (1.0f - metallic);

    // Final Radiance
    // Note: albedo / PI is the diffuse part
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

#endif // __PBR_HLSLI__
