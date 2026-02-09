#ifndef __LIGHT_INFO_HLSLI__
#define __LIGHT_INFO_HLSLI__

// LIGHT constant buffer
cbuffer DirectionalLightCB : register(BIND_DIRECTIONAL_LIGHT_CB)
{
    float3 gLightDirWS;
    int gLightEnabled;

    float3 gRadiance;
    float _padLight1;
};

#endif // __LIGHT_INFO_HLSLI__
