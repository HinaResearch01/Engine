// ================================
// DeferredCommon.hlsli
// ================================
#ifndef TSUMI_DEFERRED_COMMON_HLSLI
#define TSUMI_DEFERRED_COMMON_HLSLI

// ============================================================================
// Common Structs (全 shader 共通 / RootSignature 非依存)
// ============================================================================
struct VS_INPUT_GBUFFER
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD0;
};

struct VS_OUTPUT_GBUFFER
{
    float4 positionCS : SV_POSITION;
    float3 positionWS : TEXCOORD0;
    float3 normalWS : TEXCOORD1;
    float2 uv : TEXCOORD2;
};

struct GBUFFER_OUT
{
    float4 albedo : SV_Target0;
    float4 normalWS : SV_Target1;
    float4 reflectivity : SV_Target2;
};

struct VS_OUTPUT_FULLSCREEN
{
    float4 positionCS : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// ============================================================================
// Camera (b0)
// ============================================================================
cbuffer CameraMatricesCB : register(b0)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float4x4 gInvViewProj;
};

// ============================================================================
// Object (b10)
// ============================================================================
cbuffer ObjectCB : register(b10)
{
    float4x4 gWorld;
};

// ============================================================================
// Material (b21, t0, s0)
// ============================================================================
cbuffer MaterialParamsCB : register(b21)
{
    float3 gBaseColor;
    float  gAlpha;

    float  gReflectivity;
    float  gRoughness;
    float  gUseAlbedoTex;
    float  _padMat;
};

Texture2D    gAlbedoTex  : register(t0);
SamplerState gLinearWrap : register(s0);

// ============================================================================
// Directional Light (b30)
// ============================================================================
cbuffer DirectionalLightCB : register(b30)
{
    float3 gLightDirWS;
    int    gLightEnabled;

    float3 gRadiance;
    float  _padLight1;

    uint   gCastShadow;
    float3 _padLight2;
};

// ============================================================================
// GBuffer Inputs (t10-t13, s1)
// ============================================================================
Texture2D gGBuffer0_Albedo       : register(t10);
Texture2D gGBuffer1_NormalWS     : register(t11);
Texture2D gGBuffer2_Reflectivity : register(t12);
Texture2D gDepth01               : register(t13);
SamplerState gPointClamp         : register(s1);

// ============================================================================
// Utilities (全 shader 共通)
// ============================================================================
static float3 SafeNormalize(float3 v)
{
    float len2 = dot(v, v);
    return (len2 < 1e-12f) ? float3(0, 1, 0) : v * rsqrt(len2);
}

static VS_OUTPUT_FULLSCREEN FullscreenVS(uint vertexID)
{
    VS_OUTPUT_FULLSCREEN o;

    float2 pos =
        (vertexID == 0) ? float2(-1.0, -1.0) :
        (vertexID == 1) ? float2(-1.0, 3.0) :
                          float2(3.0, -1.0);

    o.positionCS = float4(pos, 0.0f, 1.0f);
    o.uv = pos * float2(0.5f, -0.5f) + 0.5f;
    return o;
}

static float3 ReconstructWorldPosFromDepth(float2 uv, float depth01)
{
    float2 ndc;
    ndc.x = uv.x * 2.0f - 1.0f;
    ndc.y = 1.0f - uv.y * 2.0f;

    float4 clipPos = float4(ndc, depth01, 1.0f);
    float4 worldPos = mul(gInvViewProj, clipPos);

    return worldPos.xyz / max(worldPos.w, 1e-6f);
}

#endif // TSUMI_DEFERRED_COMMON_HLSLI
