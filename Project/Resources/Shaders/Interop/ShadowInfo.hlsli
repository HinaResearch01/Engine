#ifndef __SHADOW_INFO_HLSLI__
#define __SHADOW_INFO_HLSLI__

#include "Bindings.hlsli"

static const uint CSM_CASCADE_COUNT = 4;

// Shadow constant buffer
cbuffer ShadowCB : register(BIND_SHADOW_CB)
{
    float4x4 gLightViewProj[CSM_CASCADE_COUNT];

    float4 gCascadeSplitDepths;

    float2 gShadowTexelSize; 
    float gShadowBias; 
    float gShadowNormalBias;
};

#endif // __SHADOW_INFO_HLSLI__
