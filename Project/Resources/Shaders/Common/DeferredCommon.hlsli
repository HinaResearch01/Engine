// ================================
// DeferredCommon.hlsli
// ================================
#ifndef TSUMI_DEFERRED_COMMON_HLSLI
#define TSUMI_DEFERRED_COMMON_HLSLI


// --------------------------------
// Common structs
// --------------------------------
struct VS_INPUT_GBUFFER
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT_GBUFFER
{
    float4 positionCS : SV_POSITION; // Clip
    float3 positionWS : TEXCOORD0; // WorldPos
    float3 normalWS : TEXCOORD1; // WorldNormal
    float2 uv : TEXCOORD2;
};

struct GBUFFER_OUT
{
    float4 albedo : SV_Target0; // rgb=albedo
    float4 normalWS : SV_Target1; // xyz=normalWS (-1..1)
    float4 material : SV_Target2; // r=rough g=metal b=ao a=unused
};

// Fullscreen（Lighting pass）
struct VS_OUTPUT_FULLSCREEN
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ================================
// Register Layout Policy
// ================================
//
// b0  - b9   : PerFrame / Camera 系
// b10 - b19  : PerObject 系
// b20 - b29  : Material 系
// b30 - b39  : Lighting 系
// b40 - b49  : Shadow 系
// b50 - b59  : PostProcess / Debug 系
// --------------------------------
// Constant Buffers
// --------------------------------
// Camera
cbuffer CameraMatricesCB : register(b0)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float4x4 gInvViewProj;
};

// Object
cbuffer ObjectCB : register(b10)
{
    float4x4 gWorld;
};

// Material
cbuffer MaterialUVCB : register(b20)
{
    float3x3 gUVTransform;
};

cbuffer MaterialParamsCB : register(b21)
{
    float3 gBaseColor;
    float gAlpha;

    float gRoughness;
    float gMetallic;
    float gAO;
    float gUseAlbedoTex;
};

// Light
cbuffer DirectionalLightCB : register(b30)
{
    float3 gLightDirWS;
    int gLightEnabled; // 0 or 1
    
    float3 gRadiance;
    float _pad1;

    uint gCastShadow; // 0 or 1
    float3 _pad2;
};

// --------------------------------
// Textures / Samplers
// --------------------------------
// t0  - t9   : Material textures
// t10 - t19  : GBuffer
// t20 - t29  : Shadow
// t30 - t39  : PostProcess
// Material
Texture2D gAlbedoTex : register(t0);
SamplerState gLinearWrap : register(s0);

// GBuffer
Texture2D gGBuffer0_Albedo : register(t10);
Texture2D gGBuffer1_NormalWS : register(t11);
Texture2D gGBuffer2_Material : register(t12);
Texture2D gDepth01 : register(t13);
SamplerState gPointClamp : register(s1);


// --------------------------------
// Utilities
// --------------------------------
static float3 SafeNormalize(float3 v)
{
    float len2 = dot(v, v);
    if (len2 < 1e-12f)
        return float3(0, 1, 0);
    return v * rsqrt(len2);
}

// Depth01 + invViewProj で WorldPos 復元
static float3 ReconstructWorldPosFromDepth(float2 uv, float depth01)
{
    float ndcX = uv.x * 2.0f - 1.0f;
    float ndcY = 1.0f - uv.y * 2.0f;

    float4 clip = float4(ndcX, ndcY, depth01, 1.0f);
    float4 wpos = mul(gInvViewProj, clip);
    wpos.xyz /= max(wpos.w, 1e-6f);
    return wpos.xyz;
}

// Fullscreen triangle VS（共通で使う）
static VS_OUTPUT_FULLSCREEN FullscreenVS(uint vertexID)
{
    VS_OUTPUT_FULLSCREEN o;

    // 3頂点で全画面を覆う（三角形）
    // vertexID: 0,1,2
    float2 pos = (vertexID == 0) ? float2(-1.0, -1.0) :
                 (vertexID == 1) ? float2(-1.0, 3.0) :
                                   float2(3.0, -1.0);

    float2 uv = (vertexID == 0) ? float2(0.0, 1.0) :
                 (vertexID == 1) ? float2(0.0, -1.0) :
                                   float2(2.0, 1.0);

    o.positionCS = float4(pos, 0.0, 1.0);
    o.uv = uv;
    return o;
}

#endif // TSUMI_DEFERRED_COMMON_HLSLI