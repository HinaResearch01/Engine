#ifndef __CAMERA_INFO_HLSLI__
#define __CAMERA_INFO_HLSLI__

// Camera constant buffer
cbuffer CameraCB : register(b0)
{
    float4x4 gView; // View matrix
    float4x4 gProj; // Projection matrix
    float4x4 gViewProj; // View * Projection
    float3 gCameraPosWS; // World-space camera position
    float _padding0;
};

#endif // __CAMERA_INFO_HLSLI__
