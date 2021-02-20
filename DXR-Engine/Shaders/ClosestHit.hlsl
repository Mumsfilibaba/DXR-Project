#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

ConstantBuffer<Camera> Camera : register(b0, space0);

Texture2D<float4> GBufferNormal : register(t6, space0);
Texture2D<float4> GBufferDepth : register(t7, space0);

TextureCube<float4> Skybox : register(t1, space0);

SamplerState TextureSampler : register(s0, space0);
SamplerState GBufferSampler : register(s1, space0);

RWTexture2D<float4> OutTexture : register(u0, space0);

static const float3 LightPosition = float3(0.0f, 1.0f, 0.0f);
static const float3 LightColor    = float3(1.0f, 1.0f, 1.0f);

// Local RootSignature
cbuffer MaterialBuffer : register(b0, space2)
{
    float3 Albedo;
    float  Roughness;
    float  Metallic;
    float  AO;
    int    EnableHeight;
};

Texture2D<float4> AlbedoMap    : register(t0, space2);
Texture2D<float4> NormalMap    : register(t1, space2);
Texture2D<float4> RoughnessMap : register(t2, space2);
Texture2D<float4> HeightMap    : register(t3, space2);
Texture2D<float4> MetallicMap  : register(t4, space2);
Texture2D<float4> AOMap        : register(t5, space2);

StructuredBuffer<Vertex> Vertices : register(t6, space2);
ByteAddressBuffer InIndices       : register(t7, space2);

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    // Get the base index of the triangle's first 16 bit index.
    const uint IndexSizeInBytes    = 4;
    const uint IndicesPerTriangle  = 3;
    const uint TriangleIndexStride = IndicesPerTriangle * IndexSizeInBytes;
    const uint BaseIndex           = PrimitiveIndex() * TriangleIndexStride;

    // Load up three indices for the triangle.
    uint3 Indices = InIndices.Load3(BaseIndex);

    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 TriangleNormals[3] =
    {
        Vertices[Indices[0]].Normal,
        Vertices[Indices[1]].Normal,
        Vertices[Indices[2]].Normal
    };

    float3 BarycentricCoords = float3(
        1.0f - IntersectionAttributes.barycentrics.x - IntersectionAttributes.barycentrics.y, 
        IntersectionAttributes.barycentrics.x, 
        IntersectionAttributes.barycentrics.y);
    
    float3 Normal = (TriangleNormals[0] * BarycentricCoords.x) + (TriangleNormals[1] * BarycentricCoords.y) + (TriangleNormals[2] * BarycentricCoords.z);
    Normal = normalize(Normal);
    
    float3 TriangleTangent[3] =
    {
        Vertices[Indices[0]].Tangent,
        Vertices[Indices[1]].Tangent,
        Vertices[Indices[2]].Tangent
    };

    float2 TriangleTexCoords[3] =
    {
        Vertices[Indices[0]].TexCoord,
        Vertices[Indices[1]].TexCoord,
        Vertices[Indices[2]].TexCoord
    };

    float2 TexCoords =
        (TriangleTexCoords[0] * BarycentricCoords.x) +
        (TriangleTexCoords[1] * BarycentricCoords.y) +
        (TriangleTexCoords[2] * BarycentricCoords.z);
    
    float3 Tangent =
        (TriangleTangent[0] * BarycentricCoords.x) +
        (TriangleTangent[1] * BarycentricCoords.y) +
        (TriangleTangent[2] * BarycentricCoords.z);
    Tangent = normalize(Tangent);

    float3 MappedNormal = NormalMap.SampleLevel(TextureSampler, TexCoords, 0).rgb;
    MappedNormal = UnpackNormal(MappedNormal);
    
    float3 Bitangent = normalize(cross(Normal, Tangent));
    Normal = ApplyNormalMapping(MappedNormal, Normal, Tangent, Bitangent);
    
    float3 AlbedoColor = AlbedoMap.SampleLevel(TextureSampler, TexCoords, 0).rgb;
    AlbedoColor = AlbedoColor * Albedo;
    
    // Send a new ray for reflection
    const float3 HitPosition = WorldHitPosition();
    const float3 LightDir    = normalize(LightPosition - HitPosition);
    const float3 ViewDir     = WorldRayDirection(); //normalize(Camera.Position - HitPosition);
    const float3 HalfVec     = normalize(ViewDir + LightDir);
    
    // MaterialProperties
    const float SampledAO        = AOMap.SampleLevel(TextureSampler, TexCoords, 0).r * AO;
    const float SampledMetallic  = MetallicMap.SampleLevel(TextureSampler, TexCoords, 0).r * Metallic;
    const float SampledRoughness = RoughnessMap.SampleLevel(TextureSampler, TexCoords, 0).r * Roughness;
    const float FinalRoughness   = min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    float3 ReflectedColor = Float3(0.0f);
    if (PayLoad.CurrentDepth < 4)
    {
        RayDesc Ray;
        Ray.Origin = HitPosition + (Normal * RAY_OFFSET);
        Ray.Direction = reflect(WorldRayDirection(), Normal);

        Ray.TMin = 0;
        Ray.TMax = 100000;

        RayPayload ReflectancePayLoad;
        ReflectancePayLoad.CurrentDepth = PayLoad.CurrentDepth + 1;

        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, ReflectancePayLoad);

        ReflectedColor = ReflectancePayLoad.Color;
    }
    else
    {
        ReflectedColor = Skybox.SampleLevel(TextureSampler, WorldRayDirection(), 0).rgb;
    }
    
    float3 FresnelReflect = FresnelSchlick(-WorldRayDirection(), Normal, AlbedoColor);
    ReflectedColor = FresnelReflect * ReflectedColor;

    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, AlbedoColor, SampledMetallic);

    // Reflectance equation
    float3 Lo = Float3(0.0f);

    // Calculate per-light radiance
    float  Distance    = length(LightPosition - HitPosition);
    float  Attenuation = 1.0f / (Distance * Distance);
    float3 Radiance    = LightColor * Attenuation;

    // Cook-Torrance BRDF
    float NDF = DistributionGGX(Normal, HalfVec, FinalRoughness);
    float G   = GeometrySmithGGX(Normal, LightDir, ViewDir, FinalRoughness);
    float3 F  = FresnelSchlick(ViewDir, HalfVec, F0);
    
    float3 Nominator   = NDF * G * F;
    float  Denominator = 4.0f * max(dot(Normal, ViewDir), 0.0f) * max(dot(Normal, LightDir), 0.0f);
    float3 Specular    = Nominator / max(Denominator, MIN_VALUE);
        
    // Ks is equal to Fresnel
    float3 Ks = F;
    // For energy conservation, the diffuse and specular light can't
    // be above 1.0 (unless the surface emits light); to preserve this
    // relationship the diffuse component (kD) should equal 1.0 - kS.
    float3 Kd = Float3(1.0f) - Ks;
    // Multiply kD by the inverse metalness such that only non-metals 
    // have diffuse lighting, or a linear blend if partly metal (pure metals
    // have no diffuse light).
    Kd *= 1.0f - SampledMetallic;

    // Scale light by NdotL
    float NdotL = max(dot(Normal, LightDir), 0.0f);

    // Add to outgoing radiance Lo
    Lo += (((Kd * AlbedoColor) / PI) + Specular) * Radiance * NdotL;
    
    float3 Ambient = Float3(0.03f) * AlbedoColor * SampledAO;
    float3 Color = Ambient + Lo;
    
    // Add rays together
    PayLoad.Color = Color + ReflectedColor;
}