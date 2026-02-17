#ifndef __LIGHT_INFO_HLSLI__
#define __LIGHT_INFO_HLSLI__

// LIGHT constant buffer
struct DirectionalLightCB
{
    float3 gLightDirWS;
    int gLightEnabled;

    float3 gRadiance;
    float _padLight1;

    float3 gAmbientColor;
    float _padLight2;
};

struct PointLightCB
{
    float3 gPositionWS;
    float gRange;

    float3 gRadiance;
    float _padPoint;
};

struct SpotLightCB
{
    float3 gPositionWS;
    float gRange;

    float3 gDirectionWS;
    float gInnerCos;

    float3 gRadiance;
    float gOuterCos;

    	// 拡張
	float gIntensity;
	int gShadowIndex;
	float gShadowBias;
	float _padSpot;

	float4x4 gLightViewProj;
};

#endif // __LIGHT_INFO_HLSLI__
