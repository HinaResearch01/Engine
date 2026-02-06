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
    // 1. GBuffer から情報をサンプリング
    float4 albedoData = gGBuffer0_Albedo.Sample(gPointClamp, input.uv);
    float3 albedo = albedoData.rgb;
    float alpha = albedoData.a;

    float3 normal = gGBuffer1_NormalWS.Sample(gPointClamp, input.uv).xyz;
    float4 material = gGBuffer2_Material.Sample(gPointClamp, input.uv);
    float roughness = material.r;
    float metallic = material.g;
    float ao = material.b;

    // 2. ライティング計算 (Lambert 拡散反射の例)
    // ライト方向は DirectionalLightCB (b30) から取得
    float3 L = normalize(-gLightDirWS); // ライトへの方向
    float3 N = normalize(normal);
    
    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * gRadiance * NdotL;

    // 3. 環境光 (簡易的な実装)
    float3 ambient = albedo * 0.1f * ao;

    // 最終色の出力
    float3 finalColor = diffuse + ambient;

    return float4(finalColor, alpha);
}
