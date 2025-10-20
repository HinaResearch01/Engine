#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    output.color = gTexture.Sample(gSampler, input.texcoord);
    float3 originalColor = output.color.rgb;

    float3 sepiaColor;
    sepiaColor.r = dot(originalColor, float3(0.393, 0.769, 0.189));
    sepiaColor.g = dot(originalColor, float3(0.349, 0.686, 0.168));
    sepiaColor.b = dot(originalColor, float3(0.272, 0.534, 0.131));

    output.color.rgb = sepiaColor;
    
    return output;
}