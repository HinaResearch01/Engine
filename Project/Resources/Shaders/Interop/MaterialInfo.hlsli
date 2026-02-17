#ifndef __MATERIAL_INFO_HLSLI__
#define __MATERIAL_INFO_HLSLI__

// Material constant buffer
struct MaterialParamsCB
{
    float3 gBaseColor;
    float gAlpha;

    float gRoughness;
    float gMetallic;
    float gAO; 
    float gUseAlbedoTex;
};

#endif // __MATERIAL_INFO_HLSLI__
