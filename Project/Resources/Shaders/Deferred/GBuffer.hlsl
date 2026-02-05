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

    float4 wpos = mul(float4(input.position, 1.0f), gWorld);

    o.positionWS = wpos.xyz;
    o.normalWS = SafeNormalize(mul(input.normal, (float3x3) gWorld));
    o.positionCS = mul(wpos, gViewProj);

    float3 uvh = mul(float3(input.uv, 1.0f), gUVTransform);
    o.uv = uvh.xy;

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
        albedo = gAlbedoTex.Sample(gLinearWrap, input.uv).rgb;
    }
    o.albedo = float4(albedo, gAlpha);

    // NormalWS
    float3 n = SafeNormalize(input.normalWS);
    o.normalWS = float4(n, 1.0f);

    // Material params
    o.material = float4(
        saturate(gRoughness),
        saturate(gMetallic),
        saturate(gAO),
        1.0f
    );

    return o;
}