#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Random.hlsli"

#define MAX_LIGHTS 8

#ifndef TRACE_HALF_RES
    #define TRACE_HALF_RES 0
#endif

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

Texture2DArray<float4> BlueNoiseTex : register(t6);

StructuredBuffer<RayTracingMaterial> Materials : register(t7);

StructuredBuffer<Vertex> VertexBuffers[500] : register(t8);
ByteAddressBuffer        IndexBuffers[500] : register(t508);

Texture2D<float4> MaterialTextures[256] : register(t1008);

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

SamplerState SkyboxSampler   : register(s0);
SamplerState MaterialSampler : register(s1);

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

struct PSOutput
{
    float4 ColorDepth : SV_Target0;
    float4 RayPDF     : SV_Target1;
};

PSOutput PSMain(VSOutput Input)
{
    uint Width;
    uint Height;
    GBufferDepthTex.GetDimensions(Width, Height);
    
    // Half Resolution
    uint2 TexCoord = (uint2)floor(Input.Position.xy);
#if TRACE_HALF_RES
    uint2 FullTexCoord = TexCoord * 2;
    uint  HalfWidth  = Width / 2;
    uint  HalfHeight = Height / 2;
    uint2 NoiseCoord = TexCoord / 2;
#else
    uint2 FullTexCoord = TexCoord;
    uint  HalfWidth  = Width;
    uint  HalfHeight = Height;
    uint2 NoiseCoord = TexCoord;
#endif
    
    NoiseCoord = (NoiseCoord + (RandomBuffer.FrameIndex / 2) * uint2(3, 7)) & 63;
    uint PixelIndex = (uint)(BlueNoiseTex.Load(int4(NoiseCoord, 0, 0)).r * 255.0f);
    PixelIndex = PixelIndex + RandomBuffer.FrameIndex;
    
    uint2 GBufferCoord = FullTexCoord + uint2(PixelIndex & 1, (PixelIndex >> 1) & 1);
    
    float  GBufferDepth    = GBufferDepthTex[GBufferCoord];
    float3 GBufferNormal   = GBufferNormalTex[GBufferCoord].rgb;
    float3 GBufferMaterial = GBufferMaterialTex.Load(int3(GBufferCoord, 0)).rgb;
    
    float Roughness = GBufferMaterial.r;
    
    float3 WorldPosition = PositionFromDepth(GBufferDepth, Input.TexCoord, CameraBuffer.ViewProjectionInverse);
    
    float3 N = UnpackNormal(GBufferNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    if (length(GBufferNormal) == 0.0f)
    {
        PSOutput Output;
        Output.ColorDepth = Float4(0.0f);
        Output.RayPDF     = Float4(0.0f);
        return Output;
    }
    
    uint Seed = InitRandom(TexCoord.xy, HalfWidth, RandomBuffer.FrameIndex);
    
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
    
    bool Nan = IsNan(Ray.Origin) || IsNan(Ray.Direction);
    if (!Nan)
    {
        RayQuery < RAY_FLAG_CULL_BACK_FACING_TRIANGLES > Query;
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
        
            RayTracingMaterial Material = Materials[HitData.InstanceID];
            float4 AlbedoTex = MaterialTextures[Material.AlbedoTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, 0);
            float3 HitAlbedo = ApplyGamma(AlbedoTex.rgb) * Material.Albedo;
        
            float4 NormalTex    = MaterialTextures[Material.NormalTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, 0);
            float3 MappedNormal = UnpackNormal(NormalTex.rgb);
            MappedNormal.y = -MappedNormal.y;
        
            float3 HitN = ApplyNormalMapping(MappedNormal, Vertex.Normal, Vertex.Tangent, Vertex.Bitangent);
        
            float4 MetallicTex = MaterialTextures[Material.MetallicTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, 0);
            float  HitMetallic = MetallicTex.r * Material.Metallic;
        
            float4 RoughnessTex = MaterialTextures[Material.RoughnessTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, 0);
            float  HitRoughness = clamp(RoughnessTex.r * Material.Roughness, MIN_ROUGHNESS, MAX_ROUGHNESS);
        
            float4 AOTex = MaterialTextures[Material.AOTexID].SampleLevel(MaterialSampler, Vertex.TexCoord, 0);
            float  HitAO = AOTex.r * Material.AO;
        
            float3 WorldHitPosition = Query.WorldRayOrigin() + Query.WorldRayDirection() * Query.CommittedRayT();
            FinalColor = ShadePoint(WorldHitPosition, HitN, HitV, HitAlbedo, HitAO, HitMetallic, HitRoughness);
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