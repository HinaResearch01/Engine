#include "../Field.hlsli"
#include "../../GPUParticle/GPUParticle.hlsli"
#include "../../Math/Math.hlsli"
#include "../../Random/Random.hlsli"


// パーティクル最大インスタンス数
static const uint kParticleInstanceMax = 1024;
// パーティクルの要素
RWStructuredBuffer<ParticleCS> gParticles : register(u0);

// ConstantField
ConstantBuffer<ConstantField> gField : register(b0);


[numthreads(1024, 1, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
    int particleIndex = DTid.x;
    
    // フィールド使用中
    if (gField.isUse)
    {
        // 加速度による速度変化
        gParticles[particleIndex].velocity += gField.acceleration;
        
        // 外力適用
        gParticles[particleIndex].velocity += gField.force;

        // 角速度適用（方向を考慮した影響を与える）
        gParticles[particleIndex].velocity += cross(gField.angularVelocity, gParticles[particleIndex].velocity);

        // 速度の減衰（damping は 0.0 ～ 1.0 の範囲）
        gParticles[particleIndex].velocity *= gField.damping;

        // 空気抵抗の影響（速度が速いほど抵抗が大きくなる）
        float speed = length(gParticles[particleIndex].velocity);
        if (speed > 0.0f)
        {
            float dragForce = 1.0f - gField.drag;
            gParticles[particleIndex].velocity *= max(dragForce, 0.0f);
        }
    }
}