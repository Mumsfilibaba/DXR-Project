#ifndef CASCADE_STRUCTS_HLSLI
#define CASCADE_STRUCTS_HLSLI
#include "../CoreDefines.hlsli"

struct FCascadeMatrices
{
    // 0-64
    float4x4 View;
    
    // 64-128
    float4x4 ViewProj;
    
    // 128-196
    float4x4 InvView;

    // 196-256
    float4x4 InvViewProj;
};

struct FCascadeSplit
{
    // 0-64
    float4 FrustumPlanes[NUM_FRUSTUM_PLANES];

    // 64-96
    float4 Offsets;
    float4 Scale;

    // 96-112
    float3 MinExtent;
    float  Split;

    // 112-128
    float3 MaxExtent;
    float  NearPlane;
    
    // 128-144
    float  FarPlane;
    float  MinDepth;
    float  MaxDepth;
    float  PreviousSplit;

    // 144-160
    float3 CascadeCameraPosition;
    // Reference world-space texel size for this cascade computed using the full camera clip range
    // (independent of tight-frustum depth min/max). Used for stable PCSS clamping.
    float  RefWorldTexelSize;

    // 160-176
    float  PCFMarginTexels;
    float  PCSSMarginTexels;
    float  MaxPCSSRadiusTexels;
    float  MaxPCSSRadiusWorld;

    // 176-192
    float  MaxPCSSSearchWorld;
    float  TransitionWidthViewZ;
    float  TransitionMarginTexels;
    float  CascadeUpdatedThisFrame;

    // 192-208
    float  Padding1;
    float  Padding2;
    float  Padding3;
    float  Padding4;
};

struct FCascadeGenerationInfo
{
    // 0-64
    float4x4 ShadowMatrix;

    // 64-80
    float3 LightDirection;
    float  CascadeSplitLambda;
    
    // 80-96
    float3 LightUp;
    float  CascadeResolution;

    // 96-112
    int   MaxCascadeIndex;
    int   bEnableTightFrustum;
    int   bEnableStableCascades;
    float LightPositionOffset;

    // 112-128
    float LightNearPlane;
    float LightFarPlane;
    float MaxPenumbraWorld;
    float MaxSearchDistanceWorld;

    // 128-144
    float TightFrustumShrinkFactor;
    float TightFrustumStableExtents;
    float TightFrustumDepthQuant;
    float TightFrustumForceSphereFit;

    // 144-160
    int   FilterMode;
    float PCFFilterWorld;
    float PCFMinFilterRadiusTexels;
    float AdaptiveSplitRangeEnabled;

    // 160-176
    float MaxShadowDistance;
    float ShadowPancakingEnabled;
    float Padding0;
    float Padding1;
};

struct FDirectionalLight
{
    // 0-16
    float3   Color;
    float    ShadowBias;
    
    // 16-32
    float3   Direction;
    float    Padding0;

    // 32-48
    float3   Up;
    float    LightSize;

    // 48-112
    float4x4 ShadowMatrix;
    
};

#endif
