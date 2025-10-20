#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// マスク画像
Texture2D<float> gNoiseTexture : register(t1);
SamplerState gNoiseSampler : register(s1); 

struct Material
{
    float4 color;
    // ノイズ設定
    float2 noiseScale; // ノイズのスケール（引き延ばし用）
    float noiseSpeed; // ノイズの動きの速さ
    float noiseFrequency; // ノイズの動きの頻度
    // ディスプレイスメントマップの設定
    float2 maxDisplacement; // 最大置き換え（x: 水平, y: 垂直）
    float time; // 現在の時間
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


float2 CalculateNoiseOffset(float2 uv, float time, float speed, float frequency)
{
    // ランダムな値を生成するためにノイズをサンプリング
    float randomX = gNoiseTexture.Sample(gNoiseSampler, uv * frequency + time * speed).r;
    float randomY = gNoiseTexture.Sample(gNoiseSampler, uv * frequency + time * speed + 100.0f).r;

    // ノイズ値をスケーリング
    return float2(randomX * 2.0f - 1.0f, randomY * 2.0f - 1.0f); // -1.0fから1.0fの範囲に変換
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    float2 uv = input.texcoord;

    // スケールを使用してUVを引き延ばす（数値が大きいほど引き延ばされるように計算）
    float2 scaledUV = (uv - 0.5f) / float2(gMaterial.noiseScale.x, gMaterial.noiseScale.y) + 0.5f;

    // ノイズオフセットの計算
    float2 noiseOffset = CalculateNoiseOffset(uv, gMaterial.time, gMaterial.noiseSpeed, gMaterial.noiseFrequency);
    scaledUV += noiseOffset;

    // ノイズテクスチャをサンプリング
    float noiseValue = gNoiseTexture.Sample(gNoiseSampler, scaledUV);

    // 元のテクスチャをサンプリング
    float4 originalColor = gTexture.Sample(gSampler, uv);
    //float4 originalColor = gTexture.Sample(gSampler, noiseValue);

    // ノイズを元の色に適用
    //output.color = originalColor * noiseValue;
    output.color = originalColor;

    return output;
}