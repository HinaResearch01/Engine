#ifndef __LIGHT_INFO_HLSLI__
#define __LIGHT_INFO_HLSLI__

// LIGHT constant buffer
cbuffer DirectionalLightCV : register(b20)
{
    float3 gLightDirWS;
    int gLightEnabled;

    float3 gRadiance;
    float _padLight1;

    uint gCastShadow;
    float3 _padLight2;
};

#endif // __LIGHT_INFO_HLSLI__
