#ifndef __MATERIAL_INFO_HLSLI__
#define __MATERIAL_INFO_HLSLI__

// Material constant buffer
cbuffer MaterialParamsCB : register(b20)
{
float3 gBaseColor;
float gAlpha;

float gReflectivity;
float gRoughness;
float gUseAlbedoTex;
float _padMat;
};

#endif // __MATERIAL_INFO_HLSLI__
