#ifndef __CAMERA_INFO_HLSLI__
#define __CAMERA_INFO_HLSLI__

#include "Bindings.hlsli"

// Camera constant buffer
cbuffer CameraMatricesCB : register(BIND_CAMERA_CB)
{
    float4x4 gView;
    float4x4 gProj;
    float4x4 gViewProj;
    float4x4 gInvView;
    float4x4 gInvProj;
    float4x4 gInvViewProj;
    
    float3 gCameraPosWS;
    float _pad0;
};

#endif // __CAMERA_INFO_HLSLI__
