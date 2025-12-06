#include "../GPUParticle.hlsli"


// Particleの要素
RWStructuredBuffer<ParticleCS> gParticles : register(u0);

// カメラ
ConstantBuffer<ViewProjectionMatrix> gViewProjectionMat : register(b0);


// Main
VertexShaderOutput main(VertexShaderInput input, uint instanceID : SV_InstanceID)
{
    VertexShaderOutput output;
    
    float32_t4x4 cameraMat = mul(gViewProjectionMat.view, gViewProjectionMat.projection);
  
    float4x4 wvpMat = mul(gParticles[instanceID].matWorld, cameraMat);
    
    // 頂点の位置をワールド・ビュー・プロジェクション行列で変換
    output.position = mul(input.position, wvpMat);
    
    // テクスチャ座標はそのまま渡す
    output.texCoord = input.texCoord;
    output.normal = input.normal;
    // InstanceIDはそのまま渡す
    output.instanceID = instanceID;
    
    return output;
}
