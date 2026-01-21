// ================================
// LightingDirectional.hlsl
// VS Entry: FullscreenVS
// PS Entry: LightingDirectionalPS
// ================================
#include "../Common/DeferredCommon.hlsli"

// -------------------------------
// Vertex Shader: FullscreenVS
// -------------------------------
VS_OUTPUT_FULLSCREEN FullscreenVS(uint vertexID : SV_VertexID)
{
    VS_OUTPUT_FULLSCREEN o;

    float2 pos;
    pos.x = (vertexID == 2) ? 3.0f : -1.0f;
    pos.y = (vertexID == 1) ? 3.0f : -1.0f;

    o.positionCS = float4(pos, 0.0f, 1.0f);
    o.uv = pos * 0.5f + 0.5f;

    return o;
}

// -------------------------------
// Pixel Shader: LightingDirectionalPS
// -------------------------------
float4 LightingDirectionalPS(VS_OUTPUT_FULLSCREEN input) : SV_Target0
{
    float2 uv = input.uv;

    // GBuffer read
    float3 albedo = gGBuffer0_Albedo.Sample(gPointClamp, uv).rgb;
    float3 normalWS = SafeNormalize(gGBuffer1_NormalWS.Sample(gPointClamp, uv).xyz);
    float3 mat = gGBuffer2_Material.Sample(gPointClamp, uv).rgb;

    float roughness = mat.r; // 未使用
    float metallic = mat.g; // 未使用
    float ao = mat.b;

    // Depth read
    float depth01 = gDepth01.Sample(gPointClamp, uv).r;

    // 背景（何も描かれてない）を弾く：深度1付近は空とみなす
    if (depth01 >= 0.999999f)
        return float4(0, 0, 0, 1);

    // WorldPos
    float3 worldPos = ReconstructWorldPosFromDepth(uv, depth01);

    // Directional light
    float3 L = SafeNormalize(-gLightDirWS);
    float NdotL = saturate(dot(normalWS, L));

    float3 ambient = albedo * (0.03f * ao);
    float3 diffuse = albedo * gRadiance * NdotL * ao;

    float3 color = ambient + diffuse;
    return float4(color, 1.0f);
}
