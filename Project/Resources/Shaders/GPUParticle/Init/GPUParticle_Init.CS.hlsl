#include "../GPUParticle.hlsli"
#include "../../Math/Math.hlsli"
#include "../../Random/Random.hlsli"


// パーティクル最大インスタンス数
static const uint kParticleInstanceMax = 1024;
// パーティクルの要素
RWStructuredBuffer<ParticleCS> gParticles : register(u0);

// FreeList
RWStructuredBuffer<int> gFreeList : register(u1);
// FreeListIndex
RWStructuredBuffer<int> gFreeListIndex : register(u2);


[numthreads(1024, 1, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    
    // パーティクルの初期化
    if (particleIndex < kParticleInstanceMax)
    {
        // freeListの初期化
        gFreeList[particleIndex] = particleIndex;
        
        // 全要素を0で埋める
        gParticles[particleIndex] = (ParticleCS) 0;
        gParticles[particleIndex].matWorld = Mat4x4Identity();
    }
    // カウンター初期化
    if (particleIndex == 0)
    {
        gFreeListIndex[0] = kParticleInstanceMax - 1;
    }
}
