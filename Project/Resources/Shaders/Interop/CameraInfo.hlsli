#ifndef __CAMERA_INFO_HLSLI__
#define __CAMERA_INFO_HLSLI__

// Camera constant buffer
struct CameraMatricesCB
{
    row_major float4x4 gView;
    row_major float4x4 gProj;
    row_major float4x4 gViewProj;
    row_major float4x4 gInvView;
    row_major float4x4 gInvProj;
    row_major float4x4 gInvViewProj;
    
    float3 gCameraPosWS;
    float _pad0;
};

#endif // __CAMERA_INFO_HLSLI__
