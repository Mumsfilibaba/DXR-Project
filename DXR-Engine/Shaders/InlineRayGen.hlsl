#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Random.hlsli"
#include "ShadowHelpers.hlsli"
#include "Halton.hlsli"

#define MAX_LIGHTS 64

#ifndef ENABLE_HALF_RES
    #define ENABLE_HALF_RES 1
#endif

struct VSOutput
{
    float2 TexCoord : TEXCOORD0;
    float4 Position : SV_POSITION;
};

RaytracingAccelerationStructure Scene : register(t0);

TextureCube<float4> Skybox : register(t1);

Texture2D<float4> GBufferNormalTex   : register(t2);
Texture2D<float>  GBufferDepthTex    : register(t3);
Texture2D<float4> GBufferMaterialTex : register(t4);

Texture2D<float> DirLightShadowMaps : register(t5);

Texture2DArray<float4> BlueNoiseTex : register(t6);

StructuredBuffer<RayTracingMaterial> Materials : register(t7);

Texture2D<float4> MaterialTextures[1024] : register(t8);

StructuredBuffer<Vertex> VertexBuffers[1024] : register(t1032);
ByteAddressBuffer        IndexBuffers[1024]  : register(t2056);

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

//cbuffer ShadowCastingPointLightsBuffer : register(b5, space0)
//{
//    ShadowPointLight ShadowCastingPointLights[8];
//}

//cbuffer ShadowCastingPointLightsPosRadBuffer : register(b6, space0)
//{
//    PositionRadius ShadowCastingPointLightsPosRad[8];
//}

ConstantBuffer<DirectionalLight> DirLightBuffer : register(b7, space0);

SamplerState SkyboxSampler   : register(s0);
SamplerState MaterialSampler : register(s1);

SamplerComparisonState ShadowMapSampler : register(s2);

float3 ShadePoint(float3 WorldPosition, float3 N, float3 V, float3 Albedo, float Metallic, float Roughness)
{
    float3 L0 = Float3(0.0f);
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    // Pointlights
    {
        for (uint i = 0; i < MAX_LIGHTS; i++)
        {
            if (i >= LightInfo.NumPointLights)
            {
                break;
            }
        
            const PointLight Light = PointLights[i];
            const PositionRadius LightPosRad = PointLightsPosRad[i];

            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
            L = normalize(L);

            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, Roughness, Metallic);
            
            L0 += IncidentRadiance;
        }
    }
    
    //{
    //    for (uint i = 0; i < LightInfo.NumShadowCastingPointLights; i++)
    //    {
    //        const ShadowPointLight Light     = ShadowCastingPointLights[i];
    //        const PositionRadius LightPosRad = ShadowCastingPointLightsPosRad[i];
     
    //        float3 L = LightPosRad.Position - WorldPosition;
    //        float DistanceSqrd = dot(L, L);
    //        float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
    //        L = normalize(L);
        
    //        float3 IncidentRadiance = Light.Color * Attenuation;
    //        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, FinalRoughness, Metallic);
        
    //        L0 += IncidentRadiance;
    //    }
    //}
    
    // DirectionalLights
    {
        const DirectionalLight Light = DirLightBuffer;
        const float ShadowFactor = DirectionalLightShadowFactor(DirLightShadowMaps, ShadowMapSampler, WorldPosition, N, Light);
        if (ShadowFactor > 0.001f)
        {
            float3 L = normalize(-Light.Direction);
            
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, Albedo, Roughness, Metallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }

    return L0;
}

struct PSOutput
{
    float4 ColorDepth : SV_Target0;
    float4 RayPDF     : SV_Target1;
};

PSOutput PSMain(VSOutput Input)
{
    uint Width  = 2560;
    uint Height = 1377;
    //GBufferDepthTex.GetDimensions(Width, Height);
    
    // Half Resolution
    uint2 TexCoord = (uint2)floor(Input.Position.xy);
#if ENABLE_HALF_RES
    uint2 FullTexCoord = TexCoord * 2;
    uint  HalfWidth  = Width / 2;
    uint  HalfHeight = Height / 2;
#else
    uint2 FullTexCoord = TexCoord;
    uint  HalfWidth  = Width;
    uint  HalfHeight = Height;
#endif
    
#if ENABLE_HALF_RES
    uint  FrameIndex = RandomBuffer.FrameIndex;
    uint2 NoiseCoord = TexCoord & 63;
    float BlueNoise  = BlueNoiseTex.Load(int4(NoiseCoord, FrameIndex, 0)).r;
    uint  PixelIndex = (uint)(BlueNoise * 255.0f) % 4;
#else
    uint PixelIndex = 0;
#endif
    
    uint2 GBufferCoord = FullTexCoord + uint2(PixelIndex & 1, (PixelIndex >> 1) & 1);
    
    float  GBufferDepth    = GBufferDepthTex.Load(int3(GBufferCoord, 0));
    float3 GBufferNormal   = GBufferNormalTex.Load(int3(GBufferCoord, 0)).rgb;
    float3 GBufferMaterial = GBufferMaterialTex.Load(int3(GBufferCoord, 0)).rgb;
    
    float2 UV = Input.TexCoord;
    
    float3 WorldPosition = PositionFromDepth(GBufferDepth, UV, CameraBuffer.ViewProjectionInverse);
    
    float Roughness = GBufferMaterial.r;
    
    float3 N = UnpackNormal(GBufferNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    if (length(GBufferNormal) == 0.0f)
    {
        PSOutput Output;
        Output.ColorDepth = Float4(0.0f);
        Output.RayPDF     = Float4(0.0f);
        return Output;
    }
    
    uint Seed = InitRandom(TexCoord.xy, 2560, RandomBuffer.FrameIndex);
    
    float2 Xi  = Halton23(RandomBuffer.FrameIndex);
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
        Xi   = Halton23(RandomBuffer.FrameIndex);
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
    
    bool Nan = IsNan(Ray.Origin) || IsNan(Ray.Direction);
    if (!Nan)
    {
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

            const uint BaseIndex  = CalculateBaseIndex(HitData);
            uint3 TriangleIndices = IndexBuffers[HitData.HitGroupIndex].Load3(BaseIndex);
        
            Vertex Vertices[3] =
            {
                VertexBuffers[HitData.HitGroupIndex][TriangleIndices[0]],
                VertexBuffers[HitData.HitGroupIndex][TriangleIndices[1]],
                VertexBuffers[HitData.HitGroupIndex][TriangleIndices[2]],
            };
        
            VertexData Vertex;
            LoadVertexData(HitData, Vertices, Vertex);
            
            const float3 HitV = normalize(-Query.WorldRayDirection());
            float LOD = 2.0f;
            
            RayTracingMaterial Material = Materials[HitData.InstanceID];
            
            float4 AlbedoTex = MaterialTextures[Material.AlbedoTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, LOD);
            float3 HitAlbedo = ApplyGamma(AlbedoTex.rgb) * Material.Albedo;
        
            float4 NormalTex    = MaterialTextures[Material.NormalTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, LOD);
            float3 MappedNormal = UnpackNormal(NormalTex.rgb);
            
            float3 HitN = ApplyNormalMapping(MappedNormal, Vertex.Normal, Vertex.Tangent, Vertex.Bitangent);
        
            float4 SpecularTex = MaterialTextures[Material.SpecularTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, LOD);
            float HitAO        = SpecularTex.r * Material.AO;
            float HitRoughness = clamp(SpecularTex.g * Material.Roughness, MIN_ROUGHNESS, MAX_ROUGHNESS);
            float HitMetallic  = SpecularTex.b * Material.Metallic;
        
            float3 WorldHitPosition = Ray.Origin + L * Query.CommittedRayT();
            float3 L0 = ShadePoint(WorldHitPosition, HitN, HitV, HitAlbedo, HitMetallic, HitRoughness);
            
            //float3 Emissive = Float3(0.0f);
            //if (Material.EmissiveTexID >= 0)
            //{
            //    float3 EmissiveTex = MaterialTextures[Material.EmissiveTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, LOD);
            //    Emissive = EmissiveTex.rgb;
            //}
            
            //L0 += Emissive * 2.0f;
            
            float3 Ambient = Float3(1.0f) * HitAlbedo * HitAO;
            FinalColor = Ambient + L0;
        }
        else
        {
            FinalColor = Skybox.SampleLevel(SkyboxSampler, L, 0).rgb;
        }
        
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
        
        float D = DistributionGGX(N, H, Roughness);
        float Spec_PDF = D * NdotH / (4.0f * HdotV);
       
        FinalRay = L * Query.CommittedRayT();
        FinalPDF = 1.0f / Spec_PDF;
        FinalPDF = isnan(FinalPDF) || isinf(FinalPDF) ? 0.0f : FinalPDF;
    }

    PSOutput Output;
    Output.ColorDepth = float4(FinalColor, GBufferDepth);
    Output.RayPDF     = float4(FinalRay, FinalPDF);
    return Output;
}