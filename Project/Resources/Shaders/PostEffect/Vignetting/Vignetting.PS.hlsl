#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

struct Material
{
    float4 color;
    float intensity; // ビネットの強さ (0.0 で無効、1.0 で最大強度)
    float center; // 中心位置 (0.0 から 1.0 の範囲)
    float2 radius; // 半径
    float smoothness; // ビネットのスムーズさ (0.01 で硬い、1.0 でフワッとした感じ)
    float roundness; // ビネットの丸み (0.0 で四角、1.0 で丸い)
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // テクスチャから色を取得
    float4 color = gTexture.Sample(gSampler, input.texcoord);
    
    // テクスチャの中心からの距離を計算
    float2 center = (input.texcoord - gMaterial.center) / gMaterial.radius; // X方向とY方向で異なるスケーリング
    float distance = length(center); // 楕円形状の距離を計算
    
    // `roundness` に基づいてビネットの形状を調整
    float roundedDistance = pow(distance, 1.0f - gMaterial.roundness); // 形状を調整するパラメータ
    
    // ビネットの効果を計算
    float vignette = smoothstep(1.0f - gMaterial.smoothness, 1.0f, roundedDistance);

    // ビネットの強度を掛けて色を調整
    color.rgb *= 1.0f - gMaterial.intensity * vignette;
    
    
    output.color = color;
    
    return output;
    
}