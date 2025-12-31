#ifndef CAMERA_HLSLI
#define CAMERA_HLSLI

struct CameraMatricesCB
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
    float4x4 invView;
    float4x4 invProj;
    float4x4 invViewProj;
};

struct CameraParamsCB
{
    float3 cameraPos;
    float nearPlane;
    float farPlane;
    float3 padding; // 16byte alignment
};

#endif