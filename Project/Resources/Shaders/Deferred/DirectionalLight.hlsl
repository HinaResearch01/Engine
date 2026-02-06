#include "../Common/DeferredCommon.hlsli"

// ================================
// Vertex Shader: DirectionalLight
// ================================
VS_OUTPUT_FULLSCREEN DirLightingVS(uint vertexID : SV_VertexID)
{
    return FullscreenVS(vertexID);
}

// ================================
// Pixel Shader: DirectionalLight
// ================================
float4 DirLightingPS(VS_OUTPUT_FULLSCREEN input) : SV_Target0
{
    // 1. GBufferサンプリング
    float3 albedo = gGBuffer0_Albedo.Sample(gPointClamp, input.uv).rgb;
    float3 N = normalize(gGBuffer1_NormalWS.Sample(gPointClamp, input.uv).xyz);
    float refl = gGBuffer2_Reflectivity.Sample(gPointClamp, input.uv).r;

    // 2. ライトデータ
    float3 L = normalize(-gLightDirWS + 1e-6f);
    
    // 3. ライティング計算
    // 拡散反射 (Lambert)
    float NdotL = saturate(dot(N, L));
    float3 diffuse = albedo * gRadiance * NdotL;

    // 簡易スペキュラ 
    float3 V = normalize(gInvView[3].xyz - ReconstructWorldPosFromDepth(input.uv, gDepth01.Sample(gPointClamp, input.uv).r));
    float3 H = normalize(L + V);
    float spec = pow(saturate(dot(N, H)), 32.0f) * refl;
    float3 specular = gRadiance * spec;

    // 環境光
    float3 ambient = albedo * 0.1f;

    return float4(diffuse + specular + ambient, 1.0f);
}