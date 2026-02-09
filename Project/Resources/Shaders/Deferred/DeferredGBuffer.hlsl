#include "../Core/Common.hlsli"
#include "../Core/Samplers.hlsli"
#include "../Interop/Bindings.hlsli"
#include "../Interop/CameraInfo.hlsli"
#include "../Interop/MaterialInfo.hlsli"
#include "../Interop/TransformInfo.hlsli"

// Material Texture
Texture2D gAlbedoTex : register(BIND_MATERIAL_ALBEDO_TEX);

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct VSOut
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float2 uv : TEXCOORD2;
};
struct GBuffer_Out
{
    float4 albedo : SV_Target0;
    float4 normalWS : SV_Target1;
    float4 reflectivity : SV_Target2;
};

// ================================
// Vertex Shader: GBufferVS
// ================================
VSOut GBufferVS(VSInput input)
{
    VSOut o;

    float4 wpos = mul(float4(input.position, 1.0f), gWorld);
    o.positionWS = wpos.xyz;
    o.normalWS = SafeNormalize(
        mul(input.normal, (float3x3) gWorldInvTranspose));
    o.positionCS = mul(wpos, gViewProj);
    o.uv = input.uv;
    
    return o;
}

// ================================
// Pixel Shader: GBufferPS
// ================================
GBuffer_Out GBufferPS(VSOut input)
{
    GBuffer_Out o;

    float3 albedo = gBaseColor;
    if (gUseAlbedoTex > 0.5f)
        albedo = gAlbedoTex.Sample(gLinearWrap, input.uv).rgb;

    o.albedo = float4(albedo, gAlpha);
    o.normalWS = float4(SafeNormalize(input.normalWS), 1.0f);
    o.reflectivity = float4(gReflectivity, 0, 0, 1);
    
    return o;
}