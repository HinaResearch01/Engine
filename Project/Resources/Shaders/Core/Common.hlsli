#ifndef TSUMI_CORE_COMMON_HLSLI
#define TSUMI_CORE_COMMON_HLSLI

static const float TSUMI_EPS = 1e-6f;

float3 SafeNormalize(float3 v)
{
    return (dot(v, v) > TSUMI_EPS) ? normalize(v) : float3(0, 0, 1);
}

#endif
