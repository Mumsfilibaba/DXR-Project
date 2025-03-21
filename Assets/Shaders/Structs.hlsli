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
    
    float4x4 ProjectionUnjittered;
    float4x4 ProjectionInvUnjittered;

    float3   PositionWS;
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
    // 0-16
    float3 Color;
    float  Padding0;
};

struct FShadowPointLight
{
    // 0-16
    float3 Color;
    float  ShadowBias;
    
    // 16-32
    float FarPlane;
    float Padding0;
    float Padding1;
    float Padding2;
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
    // 0-16
    float3 Albedo;
    float  Roughness;
    
    // 16-32
    float Metallic;
    float AO;
    int   Padding0;
    int   Padding1;
};

struct FLightProbeInfo
{
    // 0-16
    float3 BoxOriginWS;
    float  BoxProjection;

    // 16-32
    float3 BoxMinWS;
    float  Padding0;

    // 32-48
    float3 BoxMaxWS;
    float  Padding1;
};

#endif