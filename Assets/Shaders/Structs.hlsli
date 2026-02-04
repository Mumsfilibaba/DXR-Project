#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI
#include "CoreDefines.hlsli"

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
    // Row-major float3x4 affine transform (3 rows x 4 columns).
    row_major float3x4 Transform;

    // Inverse-transpose transform (used for normals, tangents etc).
    row_major float3x4 TransformInvT;

    uint ObjectID;
    uint Padding0;
    uint Padding1;
    uint Padding2;
};

float3 TransformPositionWS(FTransform T, float3 Position)
{
    const float4 V = float4(Position, 1.0);
    return float3(dot(V, T.Transform[0]), dot(V, T.Transform[1]), dot(V, T.Transform[2]));
}

float3 TransformDirectionWS(FTransform T, float3 Direction)
{
    const float4 V = float4(Direction, 0.0);
    return float3(dot(V, T.Transform[0]), dot(V, T.Transform[1]), dot(V, T.Transform[2]));
}

float3 TransformDirectionInvT(FTransform T, float3 Direction)
{
    const float4 V = float4(Direction, 0.0);
    return float3(dot(V, T.TransformInvT[0]), dot(V, T.TransformInvT[1]), dot(V, T.TransformInvT[2]));
}

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

    // 32-48
    float ParallaxHeightScale;
    float ParallaxMinLayers;
    float ParallaxMaxLayers;
    float Padding2;
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
