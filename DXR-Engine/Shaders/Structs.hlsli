#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

#define NUM_SHADOW_CASCADES (4)

struct ComputeShaderInput
{
    uint3 GroupID          : SV_GroupID;
    uint3 GroupThreadID    : SV_GroupThreadID;
    uint3 DispatchThreadID : SV_DispatchThreadID;
    uint  GroupIndex       : SV_GroupIndex;
};

struct Camera
{
    float4x4 ViewProjection;
    float4x4 View;
    float4x4 ViewInverse;
    float4x4 Projection;
    float4x4 ProjectionInverse;
    float4x4 ViewProjectionInverse;
    float3 Position;
    float  NearPlane;
    float3 Forward;
    float  FarPlane;
    float3 Right;
    float  AspectRatio;
};

struct PositionRadius
{
    float3 Position;
    float  Radius;
};

struct PointLight
{
    float3 Color;
    float  Padding0;
};

struct ShadowPointLight
{
    float3 Color;
    float  ShadowBias;
    float  FarPlane;
    float  MaxShadowBias;
    float  Padding0;
    float  Padding1;
};

struct SCascadeMatrices
{
    float4x4 ViewProj;
    float4x4 View;
};

struct SCascadeSplit
{
    float3 MinExtent;
    float  Split;
    float3 MaxExtent;
    float  FarPlane;
};

struct SCascadeGenerationInfo
{
    float3 LightDirection;
    float  CascadeSplitLambda;
    float3 LightUp;
    float  CascadeResolution;
};

struct DirectionalLight
{
    float3 Color;
    float  ShadowBias;
    float3 Direction;
    float  MaxShadowBias;
    float3 Up;
    float  LightSize;
};

struct Vertex
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

struct Transform
{
    float4x4 Transform;
    float4x4 TransformInv;
};

struct SMaterial
{
    float3 Albedo;
    float  Roughness;
    float  Metallic;
    float  AO;
    int    EnableHeight;
    int    EnableMask;
};

#endif