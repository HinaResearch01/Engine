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
static const float kKernel3x3[3][3] =
{
    { { 1.0f / 9.0f }, { 1.0f / 9.0f }, { 1.0f / 9.0f } },
    { { 1.0f / 9.0f }, { 1.0f / 9.0f }, { 1.0f / 9.0f } },
    { { 1.0f / 9.0f }, { 1.0f / 9.0f }, { 1.0f / 9.0f } },
};
static const float kKernel5x5[5][5] =
{
    { 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f },
    { 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f },
    { 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f },
    { 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f },
    { 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f, 1.0f / 25.0f }
};
// PI π
static const float PI = 3.14159265f;
// Gauss関数
float gauss(float x, float y, float sigma)
{
    float exponent = -(x * x + y * y) * rcp(2.0f * sigma * sigma);
    float denominator = 2.0f * PI * sigma * sigma;
    return exp(exponent) * rcp(denominator);
}


PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;
    
    // kenrnelを求める。weightは後で使う
    float weight = 0.0f;
    float kernel3x3[3][3];
    
    // uvStepSizeの算出
    uint width, height;
    gTexture.GetDimensions(width, height);
    float2 uvStepSize = float2(rcp((float) width), rcp((float) height));
    
    output.color.rgb = float3(0.0f, 0.0f, 0.0f);
    output.color.a = 1.0f;
    
    // gauss関数でkernelを求める
    for (uint i = 0; i < 3; ++i)
    {
        for (uint j = 0; j < 3; ++j)
        {
            kernel3x3[i][j] = gauss(kINdex3x3[i][j].x, kINdex3x3[i][j].y, 2.0f);
            weight += kernel3x3[i][j];
        }
    }
    
    // 求めたkernelを使い、BoxFilterと同じく畳み込みを行う。kKernel3x3と定数にしていたところがkernel3x3に代わるだけ
    for (uint k = 0; k < 3; ++k)
    {
        for (uint l = 0; l < 3; ++l)
        {
            // 3. 現在のtexcoordを算出
            float2 texcoord = input.texcoord + kINdex3x3[k][l] * uvStepSize;
          
            // 4. 色に kernel 掛けて足す
            float3 fetchColor = gTexture.Sample(gSampler, texcoord).rgb;
            output.color.rgb += fetchColor * kernel3x3[k][l];
        }
    }
    
    // 畳み込み後の値を正規化する。本来gauss関数は全体を合計すると(積分)1になるように設計されている
    // しかし、無限の範囲果たせないので、kernek値の合計である
    //　weightは1に満たない。なので合計が1になるように逆数をかけて全体を底上げして調整する
    output.color.rgb *= rcp(weight);
    
    return output;
}