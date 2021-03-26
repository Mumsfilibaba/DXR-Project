#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Constants.hlsli"

#define MAX_LIGHTS 8

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

ConstantBuffer<Camera> CameraBuffer : register(b0, space0);

ConstantBuffer<LightInfoData> LightInfo : register(b1, space0);

cbuffer PointLightsBuffer : register(b3, space0)
{
    PointLight PointLights[MAX_LIGHTS];
}

cbuffer PointLightsPosRadBuffer : register(b4, space0)
{
    PositionRadius PointLightsPosRad[MAX_LIGHTS];
}

cbuffer ShadowCastingPointLightsBuffer : register(b5, space0)
{
    ShadowPointLight ShadowCastingPointLights[8];
}

cbuffer ShadowCastingPointLightsPosRadBuffer : register(b6, space0)
{
    PositionRadius ShadowCastingPointLightsPosRad[8];
}

ConstantBuffer<DirectionalLight> DirLightBuffer : register(b7, space0);

TextureCube<float4> Skybox : register(t1, space0);

Texture2D<float4> MaterialTextures[128] : register(t7, space0);

SamplerState TextureSampler : register(s0, space0);

// Local RootSignature
StructuredBuffer<Vertex> Vertices : register(t0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
ByteAddressBuffer        Indices  : register(t1, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    // Get the base index of the triangle's first 16 bit index.
    const uint IndexSizeInBytes    = 4;
    const uint IndicesPerTriangle  = 3;
    const uint TriangleIndexStride = IndicesPerTriangle * IndexSizeInBytes;
    const uint BaseIndex           = PrimitiveIndex() * TriangleIndexStride;

    // Load up three indices for the triangle.
    uint3 TriangleIndices = Indices.Load3(BaseIndex);

    float3 BarycentricCoords = float3(
        1.0f - IntersectionAttributes.barycentrics.x - IntersectionAttributes.barycentrics.y,
        IntersectionAttributes.barycentrics.x,
        IntersectionAttributes.barycentrics.y);
    
    // Retrieve corresponding vertex normals for the triangle vertices.
    float3 TriangleNormals[3] =
    {
        Vertices[TriangleIndices[0]].Normal,
        Vertices[TriangleIndices[1]].Normal,
        Vertices[TriangleIndices[2]].Normal
    };
  
    float3 N = (TriangleNormals[0] * BarycentricCoords.x) + (TriangleNormals[1] * BarycentricCoords.y) + (TriangleNormals[2] * BarycentricCoords.z);
    N = normalize(N);
    
    //float3 TriangleTangent[3] =
    //{
    //    Vertices[TriangleIndices[0]].Tangent,
    //    Vertices[TriangleIndices[1]].Tangent,
    //    Vertices[TriangleIndices[2]].Tangent
    //};

    float2 TriangleTexCoords[3] =
    {
        Vertices[TriangleIndices[0]].TexCoord,
        Vertices[TriangleIndices[1]].TexCoord,
        Vertices[TriangleIndices[2]].TexCoord
    };

    float2 TexCoords =
        (TriangleTexCoords[0] * BarycentricCoords.x) +
        (TriangleTexCoords[1] * BarycentricCoords.y) +
        (TriangleTexCoords[2] * BarycentricCoords.z);
    TexCoords.y = 1.0f - TexCoords.y;
    
    //float3 Tangent =
    //    (TriangleTangent[0] * BarycentricCoords.x) +
    //    (TriangleTangent[1] * BarycentricCoords.y) +
    //    (TriangleTangent[2] * BarycentricCoords.z);
    //Tangent = normalize(Tangent);

    uint TextureIndex = InstanceID();
    uint AlbedoIndex  = TextureIndex;
    
    //float3 MappedNormal = MaterialTextures[NormalIndex].SampleLevel(TextureSampler, TexCoords, 0).rgb;
    //MappedNormal = UnpackNormal(MappedNormal);
    
    //float3 Bitangent = normalize(cross(N, Tangent));
    //N = ApplyNormalMapping(MappedNormal, N, Tangent, Bitangent);
    
    // TODO: Better LOD for textures
    float LOD = 0.0f;
    
    float3 AlbedoColor = ApplyGamma(MaterialTextures[AlbedoIndex].SampleLevel(TextureSampler, TexCoords, LOD).rgb);

    const float3 HitPosition = WorldHitPosition();
    const float3 V = normalize(-WorldRayDirection());
    
    // MaterialProperties
    const float AO        = 1.0f; //AOMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.AO;
    const float Metallic  = 0.0f; //MetallicMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.Metallic;
    const float Roughness = 1.0f; //RoughnessMap.SampleLevel(TextureSampler, TexCoords, 0).r * MaterialBuffer.Roughness;
    const float FinalRoughness = min(max(Roughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

    float3 L0 = (float3)0;
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, AlbedoColor, Metallic);
    
    // Pointlights
    for (uint i = 0; i < LightInfo.NumPointLights; i++)
    {
        const PointLight     Light       = PointLights[i];
        const PositionRadius LightPosRad = PointLightsPosRad[i];

        float3 L = LightPosRad.Position - HitPosition;
        float DistanceSqrd = dot(L, L);
        float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
        L = normalize(L);

        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, AlbedoColor, FinalRoughness, Metallic);
            
        L0 += IncidentRadiance;
    }
    
    for (uint i = 0; i < LightInfo.NumShadowCastingPointLights; i++)
    {
        const ShadowPointLight Light       = ShadowCastingPointLights[i];
        const PositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[i];
     
        float3 L = LightPosRad.Position - HitPosition;
        float DistanceSqrd = dot(L, L);
        float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
        L = normalize(L);
        
        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, AlbedoColor, FinalRoughness, Metallic);
        
        L0 += IncidentRadiance;
    }
    
    // DirectionalLights
    {
        const DirectionalLight Light = DirLightBuffer;
        float3 L = normalize(-Light.Direction);
            
        float3 IncidentRadiance = Light.Color;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, AlbedoColor, FinalRoughness, Metallic);
            
        L0 += IncidentRadiance;
    }

    float3 Ambient = Float3(1.0f) * AlbedoColor * AO;
    float3 Color   = Ambient + L0;
    
    PayLoad.Color = Color;
    PayLoad.T     = RayTCurrent();
}