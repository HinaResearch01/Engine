#include "Decal.hlsli"

// カメラ情報 
ConstantBuffer<ViewProjectionMatrix> gViewProjectionMat : register(b0);

// デカール設定
ConstantBuffer<DecalSettings> gDecalSettings : register(b1);

struct DecalVertexShaderInput
{
    float3 position : POSITION; 
};

VertexShaderOutput main(DecalVertexShaderInput input)
{
    VertexShaderOutput output;
    
    float4x4 decalWorldMatrix = gDecalSettings.decalMatrix; // <-- ここを修正
    
    // デカールボックスの頂点座標をワールド空間へ変換
    output.worldPos = mul(float4(input.position, 1.0f), decalWorldMatrix).xyz;

    // ワールド座標をビュープロジェクション空間へ変換
    output.position = mul(float4(output.worldPos, 1.0f), gViewProjectionMat.view);
    output.position = mul(output.position, gViewProjectionMat.projection);

    // デカールテクスチャのUV座標を計算
    // デカールボックスのローカル座標 (-1.0f から 1.0f) を 0.0f から 1.0f のUV範囲に変換
    output.decalUV = input.position.xy * 0.5f + 0.5f;

    return output;
}