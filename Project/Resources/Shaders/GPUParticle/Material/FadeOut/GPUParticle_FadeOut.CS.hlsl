#include "../../GPUParticle.hlsli"
#include "../../../Math/Math.hlsli"
#include "../../../Random/Random.hlsli"


// パーティクル最大インスタンス数
static const uint kParticleInstanceMax = 1024;

// パーティクルの要素
RWStructuredBuffer<ParticleCS> gParticles : register(u0);
// パーティクルの生存時間
RWStructuredBuffer<ParticleLifeTime> gLifeTime : register(u1);

// パーティクルのFadeOut数値
ConstantBuffer<ParticleFadeOut> gFadeOut : register(b0);


[numthreads(1024, 1, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
    uint index = DTid.x;

    //if (particleIndex >= kParticleInstanceMax)
    //    return;
    //if (gParticles[particleIndex].isAlive == 0)
    //    return;

    //// ratio（進行度）が1.0に近づくにつれてαが0に近づく
    //float ratio = gLifeTime[particleIndex].ratio;

    //// 単純な線形フェード
    //gParticles[particleIndex].color.a = 1.0f - ratio;
    
    float ratio = gLifeTime[index].ratio;
    // ratioを赤、alphaを緑に出力 → 画面で確認しやすい
    gParticles[index].color.rgb = float3(ratio, gParticles[index].color.a, 0.0f);
    gParticles[index].color.a = 1.0f - ratio;

}
