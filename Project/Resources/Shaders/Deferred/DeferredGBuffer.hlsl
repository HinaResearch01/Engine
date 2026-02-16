#include "../Core/Common.hlsli"
#include "../Interop/CameraInfo.hlsli"
#include "../Interop/MaterialInfo.hlsli"
#include "../Interop/TransformInfo.hlsli"

ConstantBuffer<CameraMatricesCB> gCamera : register(b0);
ConstantBuffer<TransformCB> gTransform : register(b1);
ConstantBuffer<MaterialParamsCB> gMaterial : register(b2);

Texture2D gAlbedoTex : register(t0);
SamplerState gLinearWrap : register(s0);

// ================================
// Structures
// ================================
struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};
struct V2P
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
// Vertex Shader
// ================================
V2P GBufferVS(VSInput input)
{
    V2P o;

    float4 wpos = mul(float4(input.position, 1.0f), gTransform.gWorld);

    o.positionWS = wpos.xyz;
    o.normalWS = SafeNormalize(
        mul(input.normal, (float3x3) gTransform.gWorldInvTranspose));
    o.positionCS = mul(wpos, gCamera.gViewProj);
    o.uv = input.uv;

    return o;
}

// ================================
// Pixel Shader
// ================================
GBuffer_Out GBufferPS(V2P input)
{
    GBuffer_Out o;

    float3 albedo = gMaterial.gBaseColor;

    if (gMaterial.gUseAlbedoTex > 0.5f)
    {
        albedo = gAlbedoTex.Sample(gLinearWrap, input.uv).rgb;
    }

    o.albedo = float4(albedo, gMaterial.gAlpha);

    o.normalWS = float4(SafeNormalize(input.normalWS), 1.0f);
    o.reflectivity = float4(gMaterial.gReflectivity, 0, 0, 1);
    
    return o;
}
