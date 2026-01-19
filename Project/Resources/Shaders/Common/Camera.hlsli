#ifndef CAMERA_HLSLI
#define CAMERA_HLSLI

struct CameraMatricesCB
{
    row_major float4x4 view;
    row_major float4x4 proj;
    row_major float4x4 viewProj;
    row_major float4x4 invView;
    row_major float4x4 invProj;
    row_major float4x4 invViewProj;
};

struct CameraParamsCB
{
    float3 cameraPos;
    float nearPlane;
    float farPlane;
    float3 padding; 
};

#endif