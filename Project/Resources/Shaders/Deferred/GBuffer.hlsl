#include "../Common/DeferredCommon.hlsli"

// ================================
// Vertex Shader: GBufferVS
// ================================
VS_OUTPUT_GBUFFER GBufferVS(VS_INPUT_GBUFFER input)
{
    VS_OUTPUT_GBUFFER o;

    float4 wpos = mul(float4(input.position, 1.0f), gWorld);

    o.positionWS = wpos.xyz;
    o.normalWS = SafeNormalize(mul(input.normal, (float3x3) gWorld));
    o.positionCS = mul(wpos, gViewProj);

    // b20廃止に伴い、一旦ストレートに渡す
    o.uv = input.uv;

    return o;
}

// ================================
// Pixel Shader: GBufferPS
// ================================
GBUFFER_OUT GBufferPS(VS_OUTPUT_GBUFFER input)
{
    GBUFFER_OUT o;

    // 1. Albedo
    float3 albedo = gBaseColor;
    if (gUseAlbedoTex > 0.5f)
    {
        albedo = gAlbedoTex.Sample(gLinearWrap, input.uv).rgb;
    }
    o.albedo = float4(albedo, gAlpha);

    // 2. Normal
    o.normalWS = float4(normalize(input.normalWS), 1.0f);

    // 3. Reflectivity
    o.reflectivity = float4(gReflectivity, 0.0f, 0.0f, 1.0f);

    return o;
}