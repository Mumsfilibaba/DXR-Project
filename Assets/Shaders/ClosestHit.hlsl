#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Constants.hlsli"

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

ConstantBuffer<FCamera> CameraBuffer : register(b0, space0);

TextureCube<float4> Skybox                : register(t1, space0);
Texture2D<float4>   MaterialTextures[128] : register(t4, space0);

SamplerState TextureSampler : register(s1, space0);

// Local RootSignature
StructuredBuffer<FVertex> Vertices  : register(t0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
ByteAddressBuffer        InIndices : register(t1, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

//ConstantBuffer<Material> MaterialBuffer : register(b0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

//Texture2D<float4> AlbedoMap    : register(t0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
//Texture2D<float4> NormalMap    : register(t1, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
//Texture2D<float4> RoughnessMap : register(t2, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
//Texture2D<float4> HeightMap    : register(t3, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
//Texture2D<float4> MetallicMap  : register(t4, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
//Texture2D<float4> AOMap        : register(t5, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

//SamplerState TextureSampler : register(s0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    PayLoad.Color        = float3(1.0f, 0.0f, 0.0f);
    PayLoad.CurrentDepth = PayLoad.CurrentDepth + 1;
    
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
    TexCoords.y = 1.0f - TexCoords.y;
    
    float3 Tangent =
        (TriangleTangent[0] * BarycentricCoords.x) +
        (TriangleTangent[1] * BarycentricCoords.y) +
        (TriangleTangent[2] * BarycentricCoords.z);
    Tangent = normalize(Tangent);

    uint TextureIndex = InstanceID();
    uint AlbedoIndex  = TextureIndex;
    uint NormalIndex  = TextureIndex + 1;
    
    float3 MappedNormal = MaterialTextures[NormalIndex].SampleLevel(TextureSampler, TexCoords, 0).rgb;
    MappedNormal = UnpackNormal(MappedNormal);
    
    float3 Bitangent = normalize(cross(Normal, Tangent));
    Normal = ApplyNormalMapping(MappedNormal, Normal, Tangent, Bitangent);
    
    float LOD = (min(RayTCurrent(), 1000.0f) / 1000.0f) * 15.0f;
    float3 AlbedoColor = ApplyGamma(MaterialTextures[AlbedoIndex].SampleLevel(TextureSampler, TexCoords, LOD).rgb);
    
    // Send a new ray for reflection
    const float3 HitPosition = WorldHitPosition();
    const float3 LightDir    = normalize(float3(0.0f, 1.0f, 0.0f));
    const float3 ViewDir     = normalize(CameraBuffer.Position - HitPosition);
    
    //// MaterialProperties
    const float SampledAO        = 1.0f; //AOMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.AO;
    const float SampledMetallic  = 1.0f; //MetallicMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.Metallic;
    const float SampledRoughness = 1.0f; //RoughnessMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.Roughness;
    const float FinalRoughness   = min(max(SampledRoughness, MIN_ROUGHNESS), MAX_ROUGHNESS);
    
    //float3 ReflectedColor = Float3(0.0f);
    //if (PayLoad.CurrentDepth < 4)
    //{
    //    RayDesc Ray;
    //    Ray.Origin    = HitPosition + (Normal * RAY_OFFSET);
    //    Ray.Direction = reflect(WorldRayDirection(), Normal);
    //    Ray.TMin      = 0;
    //    Ray.TMax      = 100000;

    //    RayPayload ReflectancePayLoad;
    //    ReflectancePayLoad.CurrentDepth = PayLoad.CurrentDepth + 1;

    //    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, ReflectancePayLoad);

    //    ReflectedColor = ReflectancePayLoad.Color;
    //}
    //else
    //{
    //    ReflectedColor = Skybox.SampleLevel(TextureSampler, WorldRayDirection(), 0).rgb;
    //}
    
    //float3 FresnelReflect = FresnelSchlick(-WorldRayDirection(), Normal, AlbedoColor);
    //ReflectedColor = FresnelReflect * ReflectedColor;

    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, AlbedoColor, SampledMetallic);

    float3 IncidentRadiance = float3(10.0f, 10.0f, 10.0f);
    float3 L0 = DirectRadiance(F0, Normal, ViewDir, LightDir, IncidentRadiance, AlbedoColor, SampledRoughness, SampledMetallic);
    
    float3 Ambient = Float3(0.03f) * AlbedoColor * SampledAO;
    float3 Color   = Ambient + L0;
    
    // Add rays together
    PayLoad.Color = Color;
}