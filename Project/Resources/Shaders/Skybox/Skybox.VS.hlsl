#include "Skybox.hlsli"

struct TransformationMatrix
{
    float4x4 World;
};
ConstantBuffer<TransformationMatrix> gTransformationMatrix : register(b0);
ConstantBuffer<TransformationViewMatrix> gTransformationViewMatrix : register(b1);

struct VertexShaderInput
{
    float4 position : POSITION0;
    float3 texcorrd : TEXCOORD0;
};


VertexShaderOutput main(VertexShaderInput input)
{
    VertexShaderOutput output;
    float4x4 resultMat;

    float4x4 cameraMat = mul(gTransformationViewMatrix.view, gTransformationViewMatrix.viewProjection);
    resultMat = mul(gTransformationMatrix.World, cameraMat);
    
    output.position = mul(input.position, resultMat).xyww;
    output.texcoord = input.position.xyz;
    
    return output;
}