#include "../PostEffect.hlsli"

Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

cbuffer gMaterial : register(b1)
{
    float4 color; // エッジカラー
    float scanlineStrength; // スキャンラインの強度
    int scanlineActive; // スキャンラインの有効フラグ
    float2 chromaOffsetR; // Rチャンネルの色収差オフセット (x, y)
    float2 chromaOffsetB; // Bチャンネルの色収差オフセット (x, y)
    int chromaActive; // 色収差の有効フラグ
    float barrelDistortion; // バレル歪みの強度
    int barrelActive; // バレル歪みの有効フラグ
    float noiseStrength; // ノイズの強度
    int noiseActive; // ノイズの有効フラグ
    float bloomStrength; // ブルームの強度
    int bloomActive; // ブルームの有効フラグ
    float2 resolution; // ウィンドウサイズ
    float time; // ゲーム開始時からのタイマー
};


// ピクセルシェーダーの出力構造体
struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

// スキャンライン効果を計算する関数
float ScanlineEffect(float2 texcoord)
{
    return sin(texcoord.y * resolution.y * 3.0f + time * 10.0f) * scanlineStrength;
}

// 色収差効果を計算する関数
float3 ChromaticAberration(float2 texcoord)
{
    float3 color;
    color.r = gTexture.Sample(gSampler, texcoord + chromaOffsetR).r; // Rチャンネルのずらし
    color.g = gTexture.Sample(gSampler, texcoord).g; // Gチャンネルはそのまま
    color.b = gTexture.Sample(gSampler, texcoord + chromaOffsetB).b; // Bチャンネルのずらし
    return color;
}

// 樽型の歪みを適用する関数（ブラウン管テレビの湾曲を再現）
float2 ApplyBarrelDistortion(float2 texcoord)
{
    float2 centeredUV = (texcoord - 0.5f) * 2.0f;
    float radius = length(centeredUV); // 中心からの距離を計算

    // バレルディストーション (中央に縮むように)
    float distortionFactor = 1.0f + barrelDistortion * (radius * radius);
    centeredUV *= distortionFactor;

    float2 distortedUV = centeredUV * 0.5f + 0.5f;
    distortedUV = clamp(distortedUV, 0.0f, 1.0f);

    return distortedUV;
}

// ノイズ効果を計算する関数
float NoiseEffect(float2 texcoord)
{
    float noise = frac(sin(dot(texcoord * resolution.xy * 0.1f, float2(12.9898f, 78.233f)) + time) * 43758.5453f);
    float scaledNoise = (noise - 0.5f) * noiseStrength;
    return scaledNoise;
}

// ブルーム効果を計算する関数
float3 BloomEffect(float3 color)
{
    float3 bloomColor = color * 1.2f; // 輝度のぼやけ
    return lerp(color, bloomColor, bloomStrength);
}

// ピクセルシェーダーのメイン関数
PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // 歪ませたUV座標でテクスチャをサンプリング
    float2 distortedUV = barrelActive ? ApplyBarrelDistortion(input.texcoord) : input.texcoord;
    output.color = gTexture.Sample(gSampler, distortedUV);

    // ブラウン管エフェクトの追加
    float scanline = scanlineActive ? ScanlineEffect(input.texcoord) : 0.0f;
    float3 chromaticColor = chromaActive ? ChromaticAberration(distortedUV) : output.color.rgb;
    float noise = noiseActive ? NoiseEffect(input.texcoord) : 0.0f;
    float3 bloomColor = bloomActive ? BloomEffect(chromaticColor) : chromaticColor;

    // 最終的な色にスキャンラインやノイズを加える
    output.color.rgb = bloomColor + scanline + noise;
    output.color.a = 1.0f;

    return output;
}