#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Random.hlsli"

#define MAX_LIGHTS 8

struct VSOutput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION;
};

RaytracingAccelerationStructure Scene : register(t0);

TextureCube<float4> Skybox : register(t1);

Texture2D<float4> GBufferAlbedoTex   : register(t2);
Texture2D<float4> GBufferNormalTex   : register(t3);
Texture2D<float>  GBufferDepthTex    : register(t4);
Texture2D<float4> GBufferMaterialTex : register(t5);

StructuredBuffer<RayTracingMaterial> Materials : register(t6);

StructuredBuffer<Vertex> Vertices[400] : register(t7);
ByteAddressBuffer        Indices[400]  : register(t407);

Texture2D<float4> MaterialTextures[128] : register(t807);

ConstantBuffer<Camera>        CameraBuffer : register(b0);
ConstantBuffer<RandomData>    RandomBuffer : register(b1);
ConstantBuffer<LightInfoData> LightInfo    : register(b2);

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

SamplerState SkyboxSampler : register(s0);

RWTexture2D<float4> ColorDepth : register(u0);
RWTexture2D<float4> RayPDF     : register(u1);

struct TriangleHit
{
    uint InstanceID;
    uint HitGroupIndex;
    uint PrimitiveIndex;
    float2 Barycentrics;
};

struct VertexData
{
    float3 Position;
    float3 Normal;
    float3 Tangent;
    float2 TexCoord;
};

void LoadModelData(in TriangleHit HitData, out VertexData Vertex)
{
    const uint IndexSizeInBytes    = 4;
    const uint IndicesPerTriangle  = 3;
    const uint TriangleIndexStride = IndicesPerTriangle * IndexSizeInBytes;
    const uint BaseIndex           = HitData.PrimitiveIndex * TriangleIndexStride;
    
    float3 BarycentricCoords = float3(1.0f - HitData.Barycentrics.x - HitData.Barycentrics.y, HitData.Barycentrics.x, HitData.Barycentrics.y);
    
    uint3 TriangleIndices = Indices[HitData.HitGroupIndex].Load3(BaseIndex);
    
    float3 TrianglePosition[3] =
    {
        Vertices[HitData.HitGroupIndex][TriangleIndices[0]].Position,
        Vertices[HitData.HitGroupIndex][TriangleIndices[1]].Position,
        Vertices[HitData.HitGroupIndex][TriangleIndices[2]].Position
    };
    
    float3 TriangleNormals[3] =
    {
        Vertices[HitData.HitGroupIndex][TriangleIndices[0]].Normal,
        Vertices[HitData.HitGroupIndex][TriangleIndices[1]].Normal,
        Vertices[HitData.HitGroupIndex][TriangleIndices[2]].Normal
    };
    
    float3 TriangleTangents[3] =
    {
        Vertices[HitData.HitGroupIndex][TriangleIndices[0]].Tangent,
        Vertices[HitData.HitGroupIndex][TriangleIndices[1]].Tangent,
        Vertices[HitData.HitGroupIndex][TriangleIndices[2]].Tangent
    };
    
    float2 TriangleTexCoords[3] =
    {
        Vertices[HitData.HitGroupIndex][TriangleIndices[0]].TexCoord,
        Vertices[HitData.HitGroupIndex][TriangleIndices[1]].TexCoord,
        Vertices[HitData.HitGroupIndex][TriangleIndices[2]].TexCoord
    };
    
    float3 Position = (TrianglePosition[0] * BarycentricCoords.x) + (TrianglePosition[1] * BarycentricCoords.y) + (TrianglePosition[2] * BarycentricCoords.z);
    float3 Normal   = (TriangleNormals[0] * BarycentricCoords.x) + (TriangleNormals[1] * BarycentricCoords.y) + (TriangleNormals[2] * BarycentricCoords.z);
    float3 Tangent  = (TriangleTangents[0] * BarycentricCoords.x) + (TriangleTangents[1] * BarycentricCoords.y) + (TriangleTangents[2] * BarycentricCoords.z);
    float2 TexCoord = (TriangleTexCoords[0] * BarycentricCoords.x) + (TriangleTexCoords[1] * BarycentricCoords.y) + (TriangleTexCoords[2] * BarycentricCoords.z);
    TexCoord.y = -TexCoord.y;
    
    Vertex.Position = Position;
    Vertex.Normal   = normalize(Normal);
    Vertex.Tangent  = normalize(Tangent);
    Vertex.TexCoord = TexCoord;
}

float3 ShadePoint(float3 WorldPosition, float3 N, float3 V, float3 Albedo, float AO, float Metallic, float Roughness)
{
    const float FinalRoughness = min(max(Roughness, MIN_ROUGHNESS), MAX_ROUGHNESS);

    float3 L0 = Float3(0.0f);
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    // Pointlights
    {
        for (uint i = 0; i < LightInfo.NumPointLights; i++)
        {
            const PointLight Light           = PointLights[i];
            const PositionRadius LightPosRad = PointLightsPosRad[i];

            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
            L = normalize(L);

            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, FinalRoughness, Metallic);
            
            L0 += IncidentRadiance;
        }
    }
    
    {
        for (uint i = 0; i < LightInfo.NumShadowCastingPointLights; i++)
        {
            const ShadowPointLight Light     = ShadowCastingPointLights[i];
            const PositionRadius LightPosRad = ShadowCastingPointLightsPosRad[i];
     
            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
            L = normalize(L);
        
            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, FinalRoughness, Metallic);
        
            L0 += IncidentRadiance;
        }
    }
    
    // DirectionalLights
    {
        const DirectionalLight Light = DirLightBuffer;
        float3 L = normalize(-Light.Direction);
            
        float3 IncidentRadiance = Light.Color;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, FinalRoughness, Metallic);
            
        L0 += IncidentRadiance;
    }

    float3 Ambient = Float3(1.0f) * Albedo * AO;
    return Ambient + L0;
}

void PSMain(VSOutput Input)
{
    uint Width;
    uint Height;
    ColorDepth.GetDimensions(Width, Height);
    
    // Half Resolution
    uint2 TexCoord     = (uint2)floor(Input.Position.xy);
    uint2 FullTexCoord = TexCoord * 2;
    
    float  GBufferDepth    = GBufferDepthTex[FullTexCoord];
    float3 GBufferNormal   = GBufferNormalTex[FullTexCoord].rgb;
    float3 GBufferMaterial = GBufferMaterialTex.Load(int3(FullTexCoord, 0)).rgb;
    
    float Roughness = GBufferMaterial.r;
    
    float3 WorldPosition = PositionFromDepth(GBufferDepth, Input.TexCoord, CameraBuffer.ViewProjectionInverse);
    
    float3 N = UnpackNormal(GBufferNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    if (length(GBufferNormal) == 0.0f)
    {
        ColorDepth[TexCoord] = Float4(0.0f);
        RayPDF[TexCoord]     = Float4(0.0f);
        return;
    }
    
    uint Seed = InitRandom(TexCoord.xy, Width, RandomBuffer.FrameIndex);
    
    float2 Xi  = Hammersley(NextRandomInt(Seed) % 8192, 8192);
    float Rnd0 = NextRandom(Seed);
    float Rnd1 = NextRandom(Seed);
    Xi.x = frac(Xi.x + Rnd0);
    Xi.y = frac(Xi.y + Rnd1);
    
    float3 H = Float3(0.0f);
    if (Roughness > 0.075f)
    {
        H = ImportanceSampleGGX(Xi, Roughness, N);
    }
    else
    {
        H = N;
    }
    
    float3 L = normalize(reflect(-V, H));
    
    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0.0f)
    {
        Xi   = Hammersley(NextRandomInt(Seed) % 8192, 8192);
        Rnd0 = NextRandom(Seed);
        Rnd1 = NextRandom(Seed);
        Xi.x = frac(Xi.x + Rnd0);
        Xi.y = frac(Xi.y + Rnd1);
    
        H = ImportanceSampleGGX(Xi, Roughness, N);
        L = normalize(reflect(-V, H));
    }
    
    float3 FinalColor = Float3(0.0f);
    float3 FinalRay   = Float3(0.0f);
    float  FinalPDF   = 0.0f;
    
    RayDesc Ray;
    Ray.Origin    = WorldPosition + (N * RAY_OFFSET);
    Ray.Direction = L;
    Ray.TMin      = 0.0f;
    Ray.TMax      = 1000.0f;
    
    RayQuery<RAY_FLAG_CULL_BACK_FACING_TRIANGLES> Query;
    Query.TraceRayInline(Scene, 0, 0xff, Ray);
    Query.Proceed();

    float3 Result = Float3(0.0f);
    if (Query.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        TriangleHit HitData;
        HitData.InstanceID     = Query.CommittedInstanceID();
        HitData.HitGroupIndex  = Query.CommittedInstanceContributionToHitGroupIndex();
        HitData.PrimitiveIndex = Query.CommittedPrimitiveIndex();
        HitData.Barycentrics   = Query.CommittedTriangleBarycentrics();

        const float3 HitV = normalize(-Query.WorldRayDirection());
        
        VertexData Vertex;
        LoadModelData(HitData, Vertex);
        
        RayTracingMaterial Material = Materials[HitData.InstanceID];
        float4 AlbedoTex = MaterialTextures[Material.AlbedoTexID].SampleLevel(SkyboxSampler, Vertex.TexCoord, 0);
        float3 HitAlbedo = AlbedoTex.rgb * Material.Albedo;
        
        float4 NormalTex    = MaterialTextures[Material.NormalTexID].SampleLevel(SkyboxSampler, Vertex.TexCoord, 0);
        float3 MappedNormal = UnpackNormal(NormalTex.rgb);
        float3 Bitangent    = normalize(cross(Vertex.Tangent, Vertex.Normal));
        float3 HitN = ApplyNormalMapping(MappedNormal, Vertex.Normal, Vertex.Tangent, Bitangent);
        
        float4 MetallicTex = MaterialTextures[Material.MetallicTexID].SampleLevel(SkyboxSampler, Vertex.TexCoord, 0);
        float  HitMetallic = MetallicTex.r * Material.Metallic;
        
        float4 RoughnessTex = MaterialTextures[Material.RoughnessTexID].SampleLevel(SkyboxSampler, Vertex.TexCoord, 0);
        float  HitRoughness = RoughnessTex.r * Material.Roughness;
        
        float4 AOTex = MaterialTextures[Material.AOTexID].SampleLevel(SkyboxSampler, Vertex.TexCoord, 0);
        float  HitAO = AOTex.r * Material.AO;
        
        float3 WorldHitPosition = Query.WorldRayOrigin() + Query.WorldRayDirection() * Query.CommittedRayT();
        FinalRay   = Query.WorldRayDirection() * Query.CommittedRayT();
        FinalColor = ShadePoint(WorldHitPosition, HitN, HitV, HitAlbedo, HitAO, HitMetallic, HitRoughness);
        
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
        
        float D = DistributionGGX(N, H, Roughness);
        float Spec_PDF = D * NdotH / (4.0f * HdotV);
       
        FinalPDF = 1.0f / Spec_PDF;
    }
    else
    {
        FinalRay   = Query.WorldRayDirection() * Query.CommittedRayT();
        FinalColor = Skybox.SampleLevel(SkyboxSampler, FinalRay, 0).rgb;
    }

    ColorDepth[TexCoord] = float4(FinalColor, GBufferDepth);
    RayPDF[TexCoord]     = float4(FinalRay, FinalPDF);
}