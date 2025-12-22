struct VSInput
{
    float3 position : POSITION0;
    float2 texCoord : TEXCOORD0;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};