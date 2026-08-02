#ifndef CASCADE_STRUCTS_HLSLI
#define CASCADE_STRUCTS_HLSLI

#include "CoreDefines.hlsli"

// Necessary to pack matrices like this in order to have it work on MoltenVK
struct FCascadeMatrices
{
    // 0-64
    float4 View[4];

    // 64-128
    float4 ViewProj[4];

    // 128-192
    float4 InvView[4];

    // 192-256
    float4 InvViewProj[4];
};

void PackMatrix(float4x4 Matrix, out float4 Rows[4])
{
    Rows[0] = Matrix[0];
    Rows[1] = Matrix[1];
    Rows[2] = Matrix[2];
    Rows[3] = Matrix[3];
}

float4x4 UnpackMatrix(float4 Rows[4])
{
    return float4x4(Rows[0], Rows[1], Rows[2], Rows[3]);
}

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
    float  Padding0;
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
    int   Padding0;
    int   Padding1;
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