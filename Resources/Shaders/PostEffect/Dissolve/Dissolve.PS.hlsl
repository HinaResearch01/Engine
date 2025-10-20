#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float threshold;
    float thinkness;
};
ConstantBuffer<Material> gMaterial : register(b1);


// マスク画像
Texture2D<float> gMaskTexture : register(t1);


struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    float mask = gMaskTexture.Sample(gSampler, input.texcoord);
    // maskの値が閾値以下の場合はdiscardして抜く
    if (mask <= gMaterial.threshold)
    {
        discard;
    }
    
    
    // エッジ作成。Edgeっぽさを算出
    float edge = 1.0f - smoothstep(gMaterial.threshold, gMaterial.threshold + gMaterial.thinkness, mask);
    output.color = gTexture.Sample(gSampler, input.texcoord);
    // Edgeっぽい程指定した色を加算
    output.color.rgb += edge * gMaterial.color.rgb;   
     
    return output;
}