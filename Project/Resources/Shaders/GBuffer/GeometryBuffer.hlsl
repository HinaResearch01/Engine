// ================================
// GBuffer.hlsl
// VS Entry: GBufferVS
// PS Entry: GBufferPS
// ================================
#include "../Common/DeferredCommon.hlsli"

// -------------------------------
// Vertex Shader: GBufferVS
// -------------------------------
VS_OUTPUT_GBUFFER GBufferVS(VS_INPUT_GBUFFER input)
{
    VS_OUTPUT_GBUFFER o;

    float4 wpos = mul(gWorld, float4(input.position, 1.0f));
    o.positionWS = wpos.xyz;

    o.normalWS = SafeNormalize(mul((float3x3) gWorld, input.normal));

    o.positionCS = mul(gViewProj, wpos);
    o.uv = input.uv;

    return o;
}

// -------------------------------
// Pixel Shader: GBufferPS
// -------------------------------
GBUFFER_OUT GBufferPS(VS_OUTPUT_GBUFFER input)
{
    GBUFFER_OUT o;

    // Albedo
    float3 albedo = gBaseColor;
    if (gUseAlbedoTex > 0.5f)
    {
        // Albedo SRV を SRGB で作っているなら Sample は線形で返る
        albedo = gAlbedoTex.Sample(gLinearWrap, input.uv).rgb;
    }
    o.albedo = float4(albedo, 1.0f);

    // NormalWS (-1..1) をそのまま格納
    float3 n = SafeNormalize(input.normalWS);
    o.normalWS = float4(n, 1.0f);

    // Material
    o.material = float4(
        saturate(gRoughness),
        saturate(gMetallic),
        saturate(gAO),
        1.0f
    );

    return o;
}