struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texcoord : TEXCOORD0;
    float3 normal : NORMAL0;
    float3 worldPosition : WORLDPOSITION0;
    float3 cameraPosition : CAMERAPOSITION0;
};

struct TransformationViewMatrix
{
    float4x4 view;
    float4x4 viewProjection;
    float4x4 orthoGraphic;
    float3 cameraPosition;
};
