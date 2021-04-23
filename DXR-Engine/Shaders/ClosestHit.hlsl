#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Constants.hlsli"

#define MAX_LIGHTS 64

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

TextureCube<float4> Skybox : register(t1, space0);

StructuredBuffer<RayTracingMaterial> Materials : register(t7);

Texture2D<float4> MaterialTextures[1024] : register(t8);

ConstantBuffer<Camera>        CameraBuffer : register(b0);
ConstantBuffer<LightInfoData> LightInfo    : register(b1);
ConstantBuffer<RandomData>    RandomBuffer : register(b2);

cbuffer PointLightsBuffer : register(b3)
{
    PointLight PointLights[MAX_LIGHTS];
}

cbuffer PointLightsPosRadBuffer : register(b4)
{
    PositionRadius PointLightsPosRad[MAX_LIGHTS];
}

//cbuffer ShadowCastingPointLightsBuffer : register(b5)
//{
//    ShadowPointLight ShadowCastingPointLights[8];
//}

//cbuffer ShadowCastingPointLightsPosRadBuffer : register(b6)
//{
//    PositionRadius ShadowCastingPointLightsPosRad[8];
//}

ConstantBuffer<DirectionalLight> DirLightBuffer : register(b7);

SamplerState TextureSampler : register(s0);

// Local RootSignature
StructuredBuffer<Vertex> VertexBuffer : register(t0, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);
ByteAddressBuffer        IndexBuffer  : register(t1, D3D12_SHADER_REGISTER_SPACE_RT_LOCAL);

[shader("closesthit")]
void ClosestHit(inout RayPayload PayLoad, in BuiltInTriangleIntersectionAttributes IntersectionAttributes)
{
    TriangleHit HitData;
    HitData.HitGroupIndex  = 0;
    HitData.Barycentrics   = IntersectionAttributes.barycentrics;
    HitData.InstanceID     = InstanceID();
    HitData.PrimitiveIndex = PrimitiveIndex();
    
    const uint BaseIndex  = CalculateBaseIndex(HitData);
    uint3 TriangleIndices = IndexBuffer.Load3(BaseIndex);

    Vertex Vertices[3] = 
    {
        VertexBuffer[TriangleIndices[0]],
        VertexBuffer[TriangleIndices[1]],
        VertexBuffer[TriangleIndices[2]],
    };
    
    VertexData Vertex;
    LoadVertexData(HitData, Vertices, Vertex);

    // TODO: Better LOD for textures
    float LOD = 0.0f;

    const float3 HitPosition = WorldHitPosition();
    const float3 V = normalize(-WorldRayDirection());
    
    // MaterialProperties
    RayTracingMaterial Material = Materials[HitData.InstanceID];
    float4 AlbedoTex = MaterialTextures[Material.AlbedoTexID].SampleLevel(TextureSampler, Vertex.TexCoord, LOD);
    float3 Albedo = ApplyGamma(AlbedoTex.rgb) * Material.Albedo;
    
    float4 NormalTex    = MaterialTextures[Material.NormalTexID].SampleLevel(TextureSampler, Vertex.TexCoord, LOD);
    float3 MappedNormal = UnpackNormal(NormalTex.rgb);
    
    float3 N = ApplyNormalMapping(MappedNormal, Vertex.Normal, Vertex.Tangent, Vertex.Bitangent);
    
    const float4 SpecularTex = MaterialTextures[Material.SpecularTexID].SampleLevel(TextureSampler, Vertex.TexCoord, LOD);
    float AO        = SpecularTex.r * Material.AO;
    float Roughness = clamp(SpecularTex.g * Material.Roughness, MIN_ROUGHNESS, MAX_ROUGHNESS);
    float Metallic  = SpecularTex.b * Material.Metallic;

    float3 L0 = Float3(0.0f);
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
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
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, Roughness, Metallic);
            
        L0 += IncidentRadiance;
    }
    
    //for (uint i = 0; i < LightInfo.NumShadowCastingPointLights; i++)
    //{
    //    const ShadowPointLight Light       = ShadowCastingPointLights[i];
    //    const PositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[i];
     
    //    float3 L = LightPosRad.Position - HitPosition;
    //    float DistanceSqrd = dot(L, L);
    //    float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
    //    L = normalize(L);
        
    //    float3 IncidentRadiance = Light.Color * Attenuation;
    //    IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, Roughness, Metallic);
        
    //    L0 += IncidentRadiance;
    //}
    
    // DirectionalLights
    {
        const DirectionalLight Light = DirLightBuffer;
        float3 L = normalize(-Light.Direction);
            
        float3 IncidentRadiance = Light.Color;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, Roughness, Metallic);
            
        L0 += IncidentRadiance;
    }

    float3 Emissive = Float3(0.0f);
    if (Material.EmissiveTexID >= 0)
    {
        float3 EmissiveTex = MaterialTextures[Material.EmissiveTexID].SampleLevel(TextureSampler, Vertex.TexCoord, 0);
        Emissive = EmissiveTex.rgb * 2.0f;
    }
    
    // Emissive
    {
        L0 += Emissive * 2.0f;
    }
    
    float3 Ambient = Float3(1.0f) * Albedo * AO;
    float3 Color   = Ambient + L0;
    
    PayLoad.Color = Color;
    PayLoad.T     = RayTCurrent();
}