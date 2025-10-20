

// Sphere Emit Data
struct SphereEmit
{
    float4 translate;
    float4 radius;
};

// Box Emit Data
struct BoxEmit
{
    float4 translate;
    float4 min;
    float4 max;
};

// Range
struct EmitRange
{
    // SR
    float4 scaleMin;
    float4 scaleMax;
    float4 rotateMin;
    float4 rotateMax;
    // Color
    float4 colorMin;
    float4 colorMax;
    // Motion
    float4 velocityMin;
    float4 velocityMax;
    // Life
    float baseLifeTime;
    float lifeTimeMin;
    float lifeTimeMax;
};

// Emittion
struct EmitConfig
{
    uint spawnCount; 
    float spawnInterval; 
    float elapsedTime; 
    uint isEmitting; 
};


// Random Seed 
struct RandomSeed
{
    float gameTime;
    float dynamicTime;
};