#ifndef __SAMPLER_HLSLI__
#define __SAMPLER_HLSLI__

SamplerState gPointClamp : register(BIND_POINT_CLAMP);
SamplerState gLinearWrap : register(BIND_LINEAR_WRAP);
SamplerState gLinearClamp : register(BIND_LINEAR_CLAMP);

#endif // __SAMPLER_HLSLI__