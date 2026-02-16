#include "../Interop/CameraInfo.hlsli"
#include "../Interop/LightInfo.hlsli"
#include "../Interop/ShadowInfo.hlsli"

#include "../Core/Common.hlsli"
#include "../Core/Fullscreen.hlsli"
#include "../Core/Math.hlsli"

#include "../Lighting/CSMShadowSampling.hlsli"
#include "../Lighting/SimpleDirectional.hlsli"

// ============================================================
// Constant Buffers
// ============================================================
ConstantBuffer<CameraMatricesCB> gCamera : register(b0);
ConstantBuffer<DirectionalLightCB> gLight : register(b1);
ConstantBuffer<ShadowCB> gShadow : register(b2);

// ============================================================
// GBuffer SRVs
// ============================================================
Texture2D gGBuffer0_Albedo : register(t0);
Texture2D gGBuffer1_NormalWS : register(t1);
Texture2D gGBuffer2_Reflectivity : register(t2);
Texture2D gDepth01 : register(t3);

// Shadow map
Texture2DArray gShadowMapCSM : register(t4);

// Sampler
SamplerState gPointClamp : register(s0);

// ============================================================
// Fullscreen VS
// ============================================================
struct VSOut
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

VSOut DirLightingVS(uint vertexID : SV_VertexID)
{
    FullscreenVSOut f = FullscreenVS(vertexID);

    VSOut o;
    o.positionCS = f.positionCS;
    o.uv = f.uv;
    return o;
}

// ============================================================
// Pixel Shader
// ============================================================
float4 DirLightingPS(VSOut i) : SV_Target
{
    float2 uv = i.uv;

    // 画面分割デバッグ (UVで領域判定)
    // ------------------------------------------------
    // | Albedo (t0)      | Normal (t1)               |
    // ------------------------------------------------
    // | Depth (t3)       | Test (Green = Running)    |
    // ------------------------------------------------

    if (uv.y < 0.5)
    {
        if (uv.x < 0.5)
        {
            // Top-Left: Albedo
            float2 subUV = uv * 2.0;
            float3 albedo = gGBuffer0_Albedo.Sample(gPointClamp, subUV).rgb;
            return float4(albedo, 1.0f);
        }
        else
        {
            // Top-Right: Normal
            float2 subUV = (uv - float2(0.5, 0.0)) * 2.0;
            float3 normal = gGBuffer1_NormalWS.Sample(gPointClamp, subUV).xyz;
            return float4((normal + 1.0f) * 0.5f, 1.0f); // -1..1 -> 0..1 for visualization
        }
    }
    else
    {
        if (uv.x < 0.5)
        {
            // Bottom-Left: Depth
            float2 subUV = (uv - float2(0.0, 0.5)) * 2.0;
            float d = gDepth01.Sample(gPointClamp, subUV).r;
            // 深度を強調表示 (0.0に近いほど黒、1.0に近いほど白)
            // リニアデプスではないので、遠くはすぐ白くなるが、0.0なら真っ黒になるはず
            return float4(d, d, d, 1.0f); 
        }
        else
        {
            // Bottom-Right: Test Connection
            // シェーダーが正常に動いているか確認用
            return float4(0.0f, 1.0f, 0.0f, 1.0f); // Green
        }
    }
}
