#ifndef STRUCTS_HLSLI
#define STRUCTS_HLSLI

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
    float4x4 PrevViewProjection;
    float4x4 View;
    float4x4 ViewInverse;
    float4x4 Projection;
    float4x4 PrevProjection;
    float4x4 ProjectionInverse;
    float4x4 ViewProjectionInverse;
    
    float3 Position;
    float  NearPlane;
    
    float3 Forward;
    float  FarPlane;
    
    float2 Jitter;
    float2 PrevJitter;
    
    float AspectRatio;
    float Width;
    float Height;
    float Padding0;
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

struct DirectionalLight
{
    float3   Color;
    float    ShadowBias;
    float3   Direction;
    float    MaxShadowBias;
    float4x4 LightMatrix;
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
    float3 Albedo;
    
    float Roughness;
    float Metallic;
    float AO;
    
    int EnableHeight;
    int EnableEmissive;
};

struct RayTracingMaterial
{
    int AlbedoTexID;
    int NormalTexID;
    int RoughnessTexID;
    int MetallicTexID;
    int AOTexID;
    
    float  Roughness;
    float  Metallic;
    float  AO;
    float3 Albedo;
    
    int Padding0;
};

struct LightInfoData
{
    int NumPointLights;
    int NumShadowCastingPointLights;
    int NumSkyLightMips;
    int Padding0;
};

#endif