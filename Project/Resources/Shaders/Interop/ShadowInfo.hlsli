#ifndef __SHADOW_INFO_HLSLI__
#define __SHADOW_INFO_HLSLI__

static const uint CSM_CASCADE_COUNT = 4;

// Shadow constant buffer
struct ShadowCB
{
    row_major float4x4 gLightViewProj[CSM_CASCADE_COUNT];

    float4 gCascadeSplitDepths;

    float2 gShadowTexelSize; 
    float gShadowBias; 
    float gShadowNormalBias;
};

#endif // __SHADOW_INFO_HLSLI__
