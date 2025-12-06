#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);
//Texture2D<float> gDepthTexture : register(t1);
//SamplerState gSamplerPoint : register(s1);

struct Material
{
    float4 color;
    float4x4 projectionInverse;
};
ConstantBuffer<Material> gMaterial : register(b1);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};


// 周囲のテクセル
static const float2 kINdex3x3[3][3] =
{
    { { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f } },
    { { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f } },
    { { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f } },
};
static const float2 kINdex5x5[5][5] =
{
    { { -2.0f, -2.0f }, { -1.0f, -2.0f }, { 0.0f, -2.0f }, { 1.0f, -2.0f }, { 2.0f, -2.0f } },
    { { -2.0f, -1.0f }, { -1.0f, -1.0f }, { 0.0f, -1.0f }, { 1.0f, -1.0f }, { 2.0f, -1.0f } },
    { { -2.0f, 0.0f }, { -1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 2.0f, 0.0f } },
    { { -2.0f, 1.0f }, { -1.0f, 1.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 2.0f, 1.0f } },
    { { -2.0f, 2.0f }, { -1.0f, 2.0f }, { 0.0f, 2.0f }, { 1.0f, 2.0f }, { 2.0f, 2.0f } }
};
// カーネル
static const float kPrewittHorizontalKernel[3][3] =
{
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
    { -1.0f / 6.0f, 0.0f, 1.0f / 6.0f },
};
static const float kPrewittVerticalKernel[3][3] =
{
    { -1.0f / 6.0f, -1.0f / 6.0f, -1.0f / 6.0f },
    { 0.0f, 0.0f, 0.0f },
    { 1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f },
};

// Luminance関数
float Luminance(float3 v)
{
    return dot(v, float3(0.2125f, 0.7154f, 0.0721f));
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // ↓↓↓↓↓↓↓↓↓ これたぶん白黒のやつ
    //// 変化の長さをウェイトとして合成。
    //// ウェイトの決定方法もいろいろと考えられる。例えばdifference.xだけ使えば横方向のエッジが検出される
    //float weight = length(difference);
    
    //// 0 ~ 1
    //weight = saturate(weight);
    
    //output.color.rgb = weight;
    //output.color.a = 1.0f;
    
    
    
    // ↓↓↓↓↓↓↓↓↓ これは輝度ベースのOutLine
    // uvStepSizeの算出
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp((float) width), rcp((float) height));
        
    // 縦横それぞれの畳み込みの結果を格納する
    float2 difference = float2(0.0f, 0.0f);
        
    // 色を輝度に変換して、畳み込みを行っていく。微分Filter用のkernelになっているので、やること自体は今までの畳み込みと同じ
    for (int x = 0; x < 3; ++x)
    {
        for (int y = 0; y < 3; ++y)
        {
            float2 texCoord = input.texcoord + kINdex3x3[x][y] * uvStepSize;
            float3 fetchColor = gTexture.Sample(gSampler, texCoord).rgb;
            float luminance = Luminance(fetchColor);
            difference.x += luminance * kPrewittHorizontalKernel[x][y];
            difference.y += luminance * kPrewittVerticalKernel[x][y];
        }
    }
        
    float weight = length(difference);
        
        // 0 ~ 1
    weight = saturate(weight * 6.0f);
        
    output.color.rgb = (1.0f - weight) * gTexture.Sample(gSampler, input.texcoord).rgb;
    output.color.a = 1.0f;
    
    
    //// 検出対象をDepthに変更する
    //// uvStepSizeの算出
    //uint width, height;
    //gTexture.GetDimensions(width, height);
    //float2 uvStepSize = float2(rcp((float) width), rcp((float) height));
        
    //// 縦横それぞれの畳み込みの結果を格納する
    //float2 difference = float2(0.0f, 0.0f);
        
    //// 色を輝度に変換して、畳み込みを行っていく。微分Filter用のkernelになっているので、やること自体は今までの畳み込みと同じ
    //for (int x = 0; x < 3; ++x)
    //{
    //    for (int y = 0; y < 3; ++y)
    //    {
    //        float2 texCoord = input.texcoord + kINdex3x3[x][y] * uvStepSize;
    //        float ndcDepth = gDepthTexture.Sample(gSamplerPoint, texCoord);
    //        // NDC -> View。P^{-1}においてxとyはzwに影響を与えないのでなんでも良い。なので、わざわざ行列を渡さなくていい。
    //        float4 viewSpace = mul(float4(0.0f, 0.0f, ndcDepth, 1.0f), gMaterial.projectionInverse);
    //        float viewZ = viewSpace.z * rcp(viewSpace.w); // 同時座標系からデカルト座標系へ変換
    //        difference.x += viewZ * kPrewittHorizontalKernel[x][y];
    //        difference.y += viewZ * kPrewittVerticalKernel[x][y];
    //    }
    //}
        
    //float weight = length(difference);
        
    //    // 0 ~ 1
    //weight = saturate(weight);
        
    //output.color.rgb = (1.0f - weight) * gDepthTexture.Sample(gSampler, input.texcoord);
    //output.color.a = 1.0f;
    
    return output;
}