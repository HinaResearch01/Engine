#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float time;
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


float rand2dTo1d(float2 value, float2 dotDir = float2(12.9898f, 78.233f))
{
    float2 smallValue = sin(value);
    float random = dot(smallValue, dotDir);
    random = frac(sin(random) * 143758.5453f);
    return random;
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // gMaterial.time を四捨五入する
    float roundedTime = round(gMaterial.time);
    
    // 乱数生成。引数にtexcoordを渡している
    float random = rand2dTo1d(input.texcoord * gMaterial.time);
    
    
    output.color = gTexture.Sample(gSampler, input.texcoord);

    // 色にする
    output.color *= random;
    output.color.a = 1.0f; // alpha値はそのまま
    
    return output;
}