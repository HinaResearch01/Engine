#include "CPUParticle.hlsli"


// トランスフォーム
StructuredBuffer<TransformationMatrix> gTransformationMat : register(t0);

// カメラ
ConstantBuffer<ViewProjectionMatrix> gViewProjectionMat : register(b0);


// Main
VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    float4x4 cameraMat = mul(gViewProjectionMat.view, gViewProjectionMat.projection);
    float4x4 wvpMat = mul(gTransformationMat[instanceID].World, cameraMat);
    
    // 頂点の位置をワールド・ビュー・プロジェクション行列で変換
    output.position = mul(input.position, wvpMat);
    
    // テクスチャ座標はそのまま渡す
    output.texCoord = input.texCoord;
    
    // 法線ベクトルをワールド行列の逆転置行列で変換し、正規化する
    output.normal = normalize(mul(input.normal, (float3x3) wvpMat));
    
    // InstanceIDはそのまま渡す
    output.instanceID = instanceID;
    
    return output;
}

