#ifndef __DEFERREDDEBUG_INFO_HLSLI__
#define __DEFERREDDEBUG_INFO_HLSLI__

// ---- Debug View Mode (Shared Contract) ----
static const uint DEBUG_VIEW_ALBEDO = 0;
static const uint DEBUG_VIEW_NORMAL = 1;
static const uint DEBUG_VIEW_MATERIAL = 2;
static const uint DEBUG_VIEW_DEPTH = 3;
static const uint DEBUG_VIEW_WORLDPOS = 4;

// ---- Debug Constant Buffer ----
cbuffer DEFERREDDEBUG : register(BIND_DEFERRED_DEBUG_CB)
{
    uint gDebugViewMode;
    float gDebugScale;
    float3 _pad;
};

#endif // __DEFERREDDEBUG_INFO_HLSLI__
