#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float grainAmount; // ノイズの強度
    float2 noiseFactors; // ノイズ生成用の係数
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャのサンプルカラーを取得
    output.color = gTexture.Sample(gSampler, input.texcoord);
    
    // uvStepSizeの算出
    uint width, height;
    gTexture.GetDimensions(width, height);

    // ノイズの導入
    // ランダムノイズを生成するために擬似乱数を使用
    uint2 pixelCoord = uint2(input.texcoord * float2(width, height));
    float noise = frac(sin(dot(pixelCoord, gMaterial.noiseFactors)) * 43758.5453);
    
    // ノイズの強度を調整
    float grainAmount = gMaterial.grainAmount; // グレインの強度を設定（0.0から1.0の範囲）

    // ノイズに基づいて色を生成
    float3 grainColor = float3(
        frac(sin(noise * 12.9898) * 43758.5453), // R成分
        frac(sin(noise * 78.233) * 43758.5453), // G成分
        frac(sin(noise * 57.5453) * 43758.5453) // B成分
    ) * grainAmount * gMaterial.color.rgb;
    
    // サンプルカラーにノイズを追加
    output.color.rgb += grainColor;
    
    return output;
}