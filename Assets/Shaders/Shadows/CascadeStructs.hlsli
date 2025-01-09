#ifndef CASCADE_STRUCTS_HLSLI
#define CASCADE_STRUCTS_HLSLI
#include "../CoreDefines.hlsli"

struct FCascadeMatrices
{
    // 0-64
    float4x4 ViewProj;
    // 64-128
    float4x4 View;
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
    int    bDepthReductionEnabled;
    int    MaxCascadeIndex;
    int    Padding0;
    int    Padding1;
};

struct FDirectionalLight
{
    // 0-16
    float3   Color;
    float    ShadowBias;
    
    // 16-32
    float3   Direction;
    float    MaxShadowBias;

    // 32-48
    float3   Up;
    float    LightSize;

    // 48-112
    float4x4 ShadowMatrix;
};

#endif