#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

struct ComputeShaderInput
{
    uint3   GroupID           : SV_GroupID;
    uint3   GroupThreadID     : SV_GroupThreadID;
    uint3   DispatchThreadID  : SV_DispatchThreadID;
    uint    GroupIndex        : SV_GroupIndex;
};

struct Camera
{
    float4x4    ViewProjection;
    float4x4    View;
    float4x4    ViewInverse;
    float4x4    Projection;
    float4x4    ProjectionInverse;
    float4x4    ViewProjectionInverse;
    float3      Position;
    float       NearPlane;
    float       FarPlane;
    float       AspectRatio;
};

struct PointLight
{
    float3  Color;
    float   ShadowBias;
    float3  Position;
    float   FarPlane;
    float   MaxShadowBias;
    float   Radius;
    float   Padding0;
    float   Padding1;
};

struct DirectionalLight
{
    float3      Color;
    float       ShadowBias;
    float3      Direction;
    float       MaxShadowBias;
    float4x4    LightMatrix;
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

struct Material
{
    float3  Albedo;
    float   Roughness;
    float   Metallic;
    float   AO;
    int     EnableHeight;
};

#endif