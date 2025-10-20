#include "Object2d.hlsli"


// マテリアル
struct Material
{
    float4 color;
    float4x4 uvTransform;
};
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);

// Dissolve
struct Dissolve
{
    int isActive;
    float threshold;
};
ConstantBuffer<Dissolve> gDissolve : register(b1);
Texture2D<float4> gMasktexture : register(t1);

SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;    
    
    // UV変換
    float4 transformedUV = mul(float4(input.texCoord, 0.0f, 1.0f), gMaterial.uvTransform);
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);
    
    // 通常のカラー処理
    output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    output.color.a = gMaterial.color.a * textureColor.a;
    
    // Dissolveが有効か確認
    if (gDissolve.isActive == 1)
    {
        // Dissolve用のマスク画像をサンプル
        float maskValue = gMasktexture.Sample(gSampler, transformedUV.xy).r;
        
        // Dissolveの閾値よりマスク値が低い場合、そのピクセルを破棄
        if (maskValue <= gDissolve.threshold)
        {
            discard;
        }
    }

    return output;
}