// ================================
// LightingDirectional.hlsl
// VS Entry: FullscreenVS
// PS Entry: LightingDirectionalPS
// ================================
#include "../Common/DeferredCommon.hlsli"

// -------------------------------
// Vertex Shader: FullscreenVS
// -------------------------------
VS_OUTPUT_FULLSCREEN LightingDirVS(uint vertexID : SV_VertexID)
{
    return FullscreenVS(vertexID);
}

// -------------------------------
// Pixel Shader: LightingDirectionalPS
// -------------------------------
float4 LightingDirPS(VS_OUTPUT_FULLSCREEN input) : SV_Target0
{
    float2 uv = input.uv;

    float4 g0 = gGBuffer0_Albedo.Sample(gPointClamp, uv);
    float4 g1 = gGBuffer1_NormalWS.Sample(gPointClamp, uv);
    float4 g2 = gGBuffer2_Material.Sample(gPointClamp, uv);
    float d = gDepth01.Sample(gPointClamp, uv).r;

    float3 albedo = g0.rgb;
    float alpha = g0.a;

    float3 n = SafeNormalize(g1.xyz);

    float rough = saturate(g2.r);
    float metal = saturate(g2.g);
    float ao = saturate(g2.b);

    // いったん最小構成：Lambert * AO
    // gLightDirWS の意味が「光が進む方向」なら L = normalize(-gLightDirWS)
    // 「ライトが向いてる方向」なら L = normalize(gLightDirWS)
    float3 L = SafeNormalize(-gLightDirWS);

    float ndotl = saturate(dot(n, L));
    float3 radiance = gRadiance;

    float3 diffuse = albedo * radiance * ndotl;
    float3 color = diffuse * ao;

    return float4(color, alpha);
}
