#include "Object3d.hlsli"


// マテリアル
struct Material
{
    float4 color;
    float4x4 uvTransform;
    uint textureIndex;
};
ConstantBuffer<Material> gMaterial : register(b0);
Texture2D<float4> gTexture : register(t0);

// カメラ
struct ViewProjectionMatrix
{
    float4x4 view;
    float4x4 projection;
    float4x4 orthoGraphic;
    float3 cameraPosition;
};
ConstantBuffer<ViewProjectionMatrix> gViewProjectionMat : register(b1);

// ライト
struct DirectionalLight
{
    float4 color;
    float3 direction;
    float intensity;
    float shininess;
    int eneble;
};
ConstantBuffer<DirectionalLight> gDirectionalLight : register(b2);

// 環境マップ
struct Environment
{
    float scale;
    int enable;
};
ConstantBuffer<Environment> gEnvironment : register(b3);
TextureCube<float4> gEnvironmentTexture : register(t1);

// 色加算
struct ColorAddition
{
    float4 addColor;
    float intensity;
    int enable;
};
ConstantBuffer<ColorAddition> gColorAddition : register(b4);

// NormalMap
struct NormalMapSettings
{
    int enable;
};
ConstantBuffer<NormalMapSettings> gNormalMapSettings : register(b5);
Texture2D<float4> gNormalMap : register(t2);

// デカール
struct DecalSettings
{
    float4x4 decalMatrix; // デカールの位置、回転、スケールを決定する行列
    float4 decalColor; // デカールの色
    float decalStrength; // デカールの適用強度
    int enable;
};
ConstantBuffer<DecalSettings> gDecalSettings : register(b6);
Texture2D<float4> gDecalTexture : register(t3);


SamplerState gSampler : register(s0);

struct PixelShaderOutput
{
    float4 color : SV_TARGET0;
};

PixelShaderOutput main(VertexShaderOutput input)
{
    PixelShaderOutput output;

    // UV変換を適用
    float4 transformedUV = mul(float4(input.texCoord, 0.0f, 1.0f), gMaterial.uvTransform);

    // テクスチャから色をサンプリング
    float4 textureColor = gTexture.Sample(gSampler, transformedUV.xy);

    // アルファ値が0.9f未満なら描画をスキップ
    if (textureColor.a == 0.0f)
    {
        discard;
    }
    
    float3 normal = normalize(input.normal);

    // --- Normal Map ---
    if (gNormalMapSettings.enable != 0)
    {
        float3 normalSample = gNormalMap.Sample(gSampler, transformedUV.xy).rgb;
        normalSample = normalSample * 2.0f - 1.0f; // [0,1] → [-1,1]
        normal = normalize(normalSample);
    }
    
    // --- Lighting ---
    if (gDirectionalLight.eneble)
    {
        // ライトの方向ベクトルを正規化
        float3 lightDir = normalize(gDirectionalLight.direction);
        
        // 視線ベクトルの計算
        float3 toEye = normalize(gViewProjectionMat.cameraPosition - input.worldPos);
        
        // リフレクトベクトルの計算
        float3 reflectLight = reflect(lightDir, normal);
        
        // ハーフベクトルの計算
        float3 halfVector = normalize(lightDir + toEye);
        
        // 拡散反射の計算
        float NdotL = dot(normal, lightDir);
        float cos = pow(NdotL * 0.5f + 0.5f, 2.0f);
        float3 diffuse = gMaterial.color.rgb * textureColor.rgb * gDirectionalLight.color.rgb * cos * gDirectionalLight.intensity;
        
        // 鏡面反射の計算
        float NdotH = dot(normal, halfVector);
        float3 specularPow = pow(saturate(NdotH), gDirectionalLight.shininess);
        float3 specular = gDirectionalLight.color.rgb * gDirectionalLight.intensity * specularPow;
       
        // 拡散反射と鏡面反射の合成
        float3 finalColor = diffuse;
        
        // テクスチャのアルファが0の場合はピクセルを破棄
        if (textureColor.a <= 0.5f)
        {
            discard;
        }
        
        // 色の設定
        output.color.rgb = finalColor;
    }
    else
    {
        // ライティングが無効な場合は、テクスチャの色にマテリアルの色を掛け合わせる
        output.color.rgb = gMaterial.color.rgb * textureColor.rgb;
    }
    
    // --- Decal ---
    if (gDecalSettings.enable != 0)
    {
        // ワールド座標からデカール空間への変換
        float4 decalPos = mul(float4(input.worldPos, 1.0f), gDecalSettings.decalMatrix);

        // デカールUV座標の計算 (0.0～1.0の範囲に正規化)
        float2 decalUV = decalPos.xy * 0.5f + 0.5f;

        // デカールが範囲外であればスキップ (デカールテクスチャの範囲外のピクセルを描画しない)
        if (decalUV.x < 0.0f || decalUV.x > 1.0f || decalUV.y < 0.0f || decalUV.y > 1.0f)
        {
            // デカールが適用されるべき領域外のピクセルは元の色を保持
        }
        else
        {
            // デカールテクスチャをサンプリング
            float4 decalSample = gDecalTexture.Sample(gSampler, decalUV);

            // デカールのアルファ値に基づいて元の色とデカールの色をブレンド
            // ひび割れ表現の場合、デカールテクスチャのひび割れ部分が不透明 (アルファ値が高い) になるようにする
            // デカールの色 (gDecalSettings.decalColor) と元のオブジェクトの色を、デカールテクスチャのアルファ値で補間する
            output.color.rgb = lerp(output.color.rgb, gDecalSettings.decalColor.rgb, decalSample.a * gDecalSettings.decalStrength);
        }
    }
    
    // --- Environment ---
    if (gEnvironment.enable)
    {
        float3 cameraToPos = normalize(input.worldPos - gViewProjectionMat.cameraPosition);
        float3 reflectedVector = reflect(cameraToPos, normalize(input.normal));
        float4 environmentColor = gEnvironmentTexture.Sample(gSampler, reflectedVector);
        environmentColor *= gEnvironment.scale;
        output.color.rgb += environmentColor.rgb;
    }
    
    // --- ColorAddition ---
    if (gColorAddition.enable)
    {
        output.color.rgb += gColorAddition.addColor.rgb * gColorAddition.intensity;
    }

    // アルファ値の設定
    output.color.a = gMaterial.color.a * textureColor.a;

    return output;
}