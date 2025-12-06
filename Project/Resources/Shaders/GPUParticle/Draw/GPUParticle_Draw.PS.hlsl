#include "../GPUParticle.hlsli"


// Particleの要素
RWStructuredBuffer<ParticleCS> gParticles : register(u0);

// テクスチャ&サンプラー
Texture2D<float4> gTexture : register(t0);
SamplerState gSampler : register(s0);

// Main
PixcelShaderOutput main(VertexShaderOutput input)
{    
    PixcelShaderOutput output;
    
    if (gParticles[input.instanceID].isAlive == 0)
    {
        discard;
    }  
    
    float4 transUV = float4(input.texCoord, 0.0f, 1.0f);
    float4 textureColor = gTexture.Sample(gSampler, input.texCoord);
    
    if (textureColor.a <= 0.01f)
    {
        discard;
    }
    output.color.rgb = textureColor.rgb * gParticles[input.instanceID].color.rgb;
    output.color.a = textureColor.a * gParticles[input.instanceID].color.a;

    return output;
}
