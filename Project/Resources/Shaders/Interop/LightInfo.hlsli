#ifndef __LIGHT_INFO_HLSLI__
#define __LIGHT_INFO_HLSLI__

// LIGHT constant buffer
struct DirectionalLightCB
{
    float3 gLightDirWS;
    int gLightEnabled;

    float3 gRadiance;
    float gIntensity;

    float3 gAmbientColor;
    float _padLight2;
};

struct PointLightCB
{
    float3 gPositionWS;
    float gRange;

    float3 gRadiance;
    float gIntensity;
};

struct SpotLightCB
{
    float3 gPositionWS;
    float gRange;

    float3 gDirectionWS;
    float gInnerCos;

    float3 gRadiance;
    float gOuterCos;

    float gIntensity;
    float3 _padSpot;
};

#endif // __LIGHT_INFO_HLSLI__
