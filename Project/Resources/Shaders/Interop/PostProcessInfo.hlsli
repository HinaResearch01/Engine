#ifndef __POSTPROCESSINFO_HLSLI__
#define __POSTPROCESSINFO_HLSLI__

// Composite / Tonemap params
struct PostCB
{
    float gExposure; // 1.0 = no change
    float gGamma; // usually 2.2
    uint gTonemapMode; // 0=none, 1=Reinhard, 2=ACES-ish
    float _pad0;
};

#endif
