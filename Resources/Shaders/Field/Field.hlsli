// ConstantFieldの構造体
struct ConstantField
{
    float3 acceleration; // 加速度
    float damping; // 速度の減衰率（0.0～1.0）
    float3 angularVelocity; // 角速度
    float drag; // 空気抵抗
    float3 force; // 外力
    float mass; // 質量
    uint isUse; // 使用するか
};