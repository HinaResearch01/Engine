
struct VertexShaderOutput
{
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
    uint instanceID : INSTANCEID0;
};

struct VertexShaderInput
{
    float4 position : POSITION0;
    float2 texCoord : TEXCOORD0;
    float3 normal : NORMAL0;
};

struct PixcelShaderOutput
{
    float4 color : SV_TARGET0;
};

// 生存時間に関する構造体
struct ParticleLifeTime
{
    float current;
    float initTime;
    float ratio;
};

// Particleの見た目関連の構造体
struct ParticleVisual
{
    float4 color;
};

// パーティクルの挙動に関する構造体
struct ParticleMotion
{
    float3 acceleration; // 加速度
    float damping; // 速度の減衰率（0.0～1.0）
    float3 angularVelocity; // 角速度
    float3 force; // 外力
    float drag; // 空気抵抗
};

// ParticleのFadeOutに関する構造体
struct ParticleFadeOut
{
    float fadeStart; // フェード開始時の ratio（例: 0.7）
    float fadeEnd; // フェード終了時の ratio（例: 1.0）
    float fadePower; // フェードの強さ（ease調整用、例: 1.0 = 線形, 2.0 = 緩やか, 0.5 = 急）
};


// Particleを表す構造体
struct ParticleCS
{
    float3 scale;
    float3 rotate;
    float3 translate;
    float4x4 matWorld;
    float4 color;
    float3 velocity;
    uint isAlive;
};



// PreView
struct PreView
{
    float4x4 viewProjection;
    float4x4 billboardMatrix;
};

// カメラ
struct ViewProjectionMatrix
{
    float4x4 view;
    float4x4 projection;
    float4x4 orthoGraphic;
    float3 cameraPosition;
};
