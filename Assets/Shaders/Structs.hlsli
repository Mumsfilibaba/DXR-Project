#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI
#include "CoreDefines.hlsli"

struct FComputeShaderInput
{
    uint3 GroupID          : SV_GroupID;
    uint3 GroupThreadID    : SV_GroupThreadID;
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint  GroupIndex       : SV_GroupIndex;
};

struct FCamera
{
    float4x4 PrevViewProjection;

    float4x4 ViewProjection;
    float4x4 ViewProjectionInv;

    float4x4 ViewProjectionUnjittered;
    float4x4 ViewProjectionInvUnjittered;

    float4x4 View;
    float4x4 ViewInv;

    float4x4 Projection;
    float4x4 ProjectionInv;
    
    float3   Position;
    float    NearPlane;
    
    float3   Forward;
    float    FarPlane;

    float3   Right;
    float    AspectRatio;

    float2   Jitter;
    float2   PrevJitter;

    float    ViewportWidth;
    float    ViewportHeight;
    float    Padding0;
    float    Padding1;
};

struct FPositionRadius
{
    float3 Position;
    float  Radius;
};

struct FPointLight
{
    float3 Color;
    float  Padding0;
};

struct FShadowPointLight
{
    float3 Color;
    float  ShadowBias;
    float  FarPlane;
    float  MaxShadowBias;
    float  Padding0;
    float  Padding1;
};

struct FCascadeMatrices
{
    float4x4 ViewProj;
    float4x4 View;
};

struct FCascadeSplit
{
    float4 FrustumPlanes[NUM_FRUSTUM_PLANES];

    float4 Offsets;
    float4 Scale;

    float3 MinExtent;
    float  Split;

    float3 MaxExtent;
    float  NearPlane;
    
    float  FarPlane;
    float  MinDepth;
    float  MaxDepth;
    float  PreviousSplit;
};

struct FCascadeGenerationInfo
{
    float4x4 ShadowMatrix;

    float3 LightDirection;
    float  CascadeSplitLambda;
    
    float3 LightUp;
    float  CascadeResolution;

    int    bDepthReductionEnabled;
    int    MaxCascadeIndex;
    int    Padding0;
    int    Padding1;
};

struct FDirectionalLight
{
    float3   Color;
    float    ShadowBias;
    
    float3   Direction;
    float    MaxShadowBias;

    float3   Up;
    float    LightSize;

    float4x4 ShadowMatrix;
};

struct FVertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

struct FTransform
{
    float4x4 Transform;
    float4x4 TransformInv;
};

struct FMaterial
{
    float3 Albedo;
    float  Roughness;
    
    float  Metallic;
    float  AO;
    int    Padding0;
    int    Padding1;
};

#endif