#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

#include "CoreDefines.hlsli"

struct FCamera
{
    // 0-64
    float4x4 PrevViewProjection;

    // 64-128
    float4x4 ViewProjection;
    // 128-192
    float4x4 ViewProjectionInv;

    // 192-256
    float4x4 ViewProjectionUnjittered;
    // 256-320
    float4x4 ViewProjectionInvUnjittered;

    // 320-384
    float4x4 View;
    // 384-448
    float4x4 ViewInv;

    // 448-512
    float4x4 Projection;
    // 512-576
    float4x4 ProjectionInv;

    // 576-640
    float4x4 ProjectionUnjittered;
    // 640-704
    float4x4 ProjectionInvUnjittered;

    // 704-720
    float3   PositionWS;
    float    NearPlane;

    // 720-736
    float3   Forward;
    float    FarPlane;

    // 736-752
    float3   Right;
    float    AspectRatio;

    // 752-768
    // Offset baked into ViewProjection/PrevViewProjection. Zero when the rasterizer applies the jitter.
    float2   ProjectionJitter;
    float2   PrevProjectionJitter;

    // 768-784
    float    ViewportWidth;
    float    ViewportHeight;
    // Offset present in the rendered image, however it was produced.
    float2   ImageJitter;

    // 784-800
    float3   PrevPositionWS;
    float    Padding2;
};

struct FPositionRadius
{
    // 0-16
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
    // 0-12
    float3 Position;
    // 12-24
    float3 Normal;
    // 24-36
    float3 Tangent;
    // 36-44
    float2 TexCoord;
};

struct FPerObject
{
    // 0-48
    row_major float3x4 LocalToWorld;
    // 48-96
    row_major float3x4 TransformInvT;

    // 96-112
    uint ObjectID;
    uint MaterialIndex;
    uint Padding1;
    uint Padding2;
};

struct FMaterial
{
    // 0-16
    float3 Albedo;
    float  Roughness;
    
    // 16-32
    float Metallic;
    float AO;
    uint  AlbedoHandle;
    uint  NormalHandle;

    // 32-48
    float ParallaxHeightScale;
    float ParallaxMinLayers;
    float ParallaxMaxLayers;
    uint  MaterialHandle;

    // 48-64
    uint  HeightHandle;
    uint  SamplerHandle;
    uint  NormalMapFlags;
    uint  Padding0;
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
