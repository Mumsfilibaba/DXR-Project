#include "PBRHelpers.hlsli"
#include "ShadowHelpers.hlsli"
#include "Helpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Poisson.hlsli"

#define THREAD_COUNT        16
#define TOTAL_THREAD_COUNT  (THREAD_COUNT * THREAD_COUNT)
#define MAX_LIGHTS_PER_TILE 1024

#ifdef DRAW_TILE_DEBUG 
    #define DRAW_TILE_OCCUPANCY 1
#else
    #define DRAW_TILE_OCCUPANCY 0
#endif

//#define DRAW_CASCADE_DEBUG
#ifdef DRAW_CASCADE_DEBUG
    #define DRAW_SHADOW_CASCADE 1
#else
    #define DRAW_SHADOW_CASCADE 0
#endif

#define BLEND_CASCADES  (1)
#define ROTATE_SAMPLES  (1)
#define ENABLE_PCSS     (0)
#define BAND_PERCENTAGE (0.15f)

// G-Buffer
Texture2D<float4> AlbedoTex       : register(t0);
Texture2D<float4> NormalTex       : register(t1);
Texture2D<float4> MaterialTex     : register(t2);
Texture2D<float>  DepthStencilTex : register(t3);

// Reflections
Texture2D<float4> DXRReflection : register(t4);

// Reflection probe
TextureCube<float4> IrradianceMap         : register(t5);
TextureCube<float4> SpecularIrradianceMap : register(t6);
Texture2D<float4>   IntegrationLUT        : register(t7);

// Shadow Cascade
Texture2D<float> ShadowCascade0 : register(t8);
Texture2D<float> ShadowCascade1 : register(t9);
Texture2D<float> ShadowCascade2 : register(t10);
Texture2D<float> ShadowCascade3 : register(t11);

// Point Shadows
TextureCubeArray<float> PointLightShadowMaps : register(t12);

// SSAO
Texture2D<float3> SSAO : register(t13);

// Shadow Cascade Data
StructuredBuffer<SCascadeMatrices> CascadeMatrixBuffer : register(t14);
StructuredBuffer<SCascadeSplit>    CascadeSplitsBuffer : register(t15);

SamplerState LUTSampler        : register(s0);
SamplerState IrradianceSampler : register(s1);

SamplerComparisonState ShadowMapSampler0 : register(s2); // Point-Lights

SamplerState DirectionalLightSampler : register(s3); // Directional-Lights

cbuffer Constants : register(b0, D3D12_SHADER_REGISTER_SPACE_32BIT_CONSTANTS)
{
    int NumPointLights;
    int NumShadowCastingPointLights;
    int NumSkyLightMips;
    int ScreenWidth;
    int ScreenHeight;
};

ConstantBuffer<Camera> CameraBuffer : register(b0, space0);

cbuffer PointLightsBuffer : register(b1, space0)
{
    PointLight PointLights[MAX_LIGHTS_PER_TILE];
}

cbuffer PointLightsPosRadBuffer : register(b2, space0)
{
    PositionRadius PointLightsPosRad[MAX_LIGHTS_PER_TILE];
}

cbuffer ShadowCastingPointLightsBuffer : register(b3, space0)
{
    ShadowPointLight ShadowCastingPointLights[8];
}

cbuffer ShadowCastingPointLightsPosRadBuffer : register(b4, space0)
{
    PositionRadius ShadowCastingPointLightsPosRad[8];
}

ConstantBuffer<DirectionalLight> DirLightBuffer : register(b5, space0);

RWTexture2D<float4> Output : register(u0, space0);

// Cascaded Shadow Mapping
#define NUM_BLOCKER_SAMPLES (128)
#define NUM_PCF_SAMPLES     (128)

// From: http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float LightSize, float LightNear, float z)
{
    return LightSize * 0.5f * (z - LightNear) / z;
}

float FindBlockerDistance(in Texture2D<float> ShadowMap, float2 TexCoords, float CompareDepth, float SearchRadius, float Scale, inout uint RandomSeed)
{
    int NumBlockers = 0;
    float AvgBlockerDistance = 0;
    
    for (int i = 0; i < NUM_BLOCKER_SAMPLES; i++)
    {
        float2 RandomDirection;
        if (NUM_BLOCKER_SAMPLES == 16)
        {
            RandomDirection = PoissonDisk16[i];
        }
        else if (NUM_BLOCKER_SAMPLES == 32)
        {
            RandomDirection = PoissonDisk32[i];
        }
        else if (NUM_BLOCKER_SAMPLES == 64)
        {
            RandomDirection = PoissonDisk64[i];
        }
        else if (NUM_BLOCKER_SAMPLES == 128)
        {
            RandomDirection = PoissonDisk128[i];
        }
        
        RandomDirection = RandomDirection * SearchRadius * Scale;
        
        float Depth = ShadowMap.SampleLevel(DirectionalLightSampler, TexCoords.xy + RandomDirection, 0.0f);
        if (Depth < CompareDepth)
        {
            NumBlockers++;
            AvgBlockerDistance += Depth;
        }
    }
    
    if (NumBlockers > 0)
    {
        return AvgBlockerDistance / float(NumBlockers);
    }
    else
    {
        return -1.0f;
    }
}

float PCFDirectionalLight(in Texture2D<float> ShadowMap, float2 TexCoords, float CompareDepth, float SearchRadius, float Scale, inout uint RandomSeed)
{
    float Shadow = 0;
    for (int i = 0; i < NUM_PCF_SAMPLES; i++)
    {
        float2 RandomDirection;
        if (NUM_PCF_SAMPLES == 16)
        {
            RandomDirection = PoissonDisk16[i];
        }
        else if (NUM_PCF_SAMPLES == 32)
        {
            RandomDirection = PoissonDisk32[i];
        }
        else if (NUM_PCF_SAMPLES == 64)
        {
            RandomDirection = PoissonDisk64[i];
        }
        else if (NUM_PCF_SAMPLES == 128)
        {
            RandomDirection = PoissonDisk128[i];
        }

#if ROTATE_SAMPLES
        RandomDirection = OneToMinusOne(CranleyPatterssonRotation(MinusOneToOne(RandomDirection), RandomSeed)) * SearchRadius;
#else
        RandomDirection = RandomDirection * SearchRadius;
#endif
        
#if ENABLE_PCSS == 0
        RandomDirection = RandomDirection * Scale;
#endif
        
        float Depth = ShadowMap.SampleLevel(DirectionalLightSampler, TexCoords.xy + RandomDirection, 0.0f);
        if (Depth < CompareDepth)
        {
            Shadow += 1.0f;
        }
    }
    
    return 1.0f - saturate(Shadow / float(NUM_PCF_SAMPLES));
}

float CalcPenumbraWidth(float LightSize, float ZReciver, float ZBlocker)
{
    return abs(LightSize * (ZReciver - ZBlocker) / ZBlocker);
}

float CalcFilterRadius(float PenumbraWidth, float LightSize, float LightNear, float z)
{
    return PenumbraWidth * LightSize * LightNear / z;
}

float PCSSDirectionalLight(
    in Texture2D<float> ShadowMap,
    float3 ShadowCoords,
    float3 LightViewPos,
    float LightSize,
    float LightNear,
    float LightFar,
    float Scale,
    float Bias,
    inout uint RandomSeed)
{
    // TODO: Correct Sampler instead
    if (ShadowCoords.x > 1.0f || ShadowCoords.y > 1.0f || ShadowCoords.z > 1.0f || ShadowCoords.x < 0.0f || ShadowCoords.y < 0.0f || ShadowCoords.z < 0.0f)
    {
        return 1.0f;
    }
    
    float CompareDepth = (ShadowCoords.z - Bias);
    
#if ENABLE_PCSS
    float BlockerSearchRadius = SearchWidth(LightSize, LightNear, LightViewPos.z);
    float BlockerDistance = FindBlockerDistance(ShadowMap, ShadowCoords.xy, CompareDepth, BlockerSearchRadius, Scale, RandomSeed);
    if (BlockerDistance < 0.0f)
    {
        return 1.0f;
    }

    float DepthRange = LightFar - LightNear;
    float AvgBlockerZ   = BlockerDistance * DepthRange;
    float PenumbraWidth = CalcPenumbraWidth(LightSize, LightViewPos.z, AvgBlockerZ);
    
    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, PenumbraWidth, Scale, RandomSeed);
#else
    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, 0.002f, Scale, RandomSeed);
#endif
}

float3 GetShadowCoords(uint CascadeIndex, float3 World)
{
    float4 LightClipSpacePos = mul(float4(World, 1.0f), CascadeMatrixBuffer[CascadeIndex].ViewProj);
    
    float3 ProjCoords = LightClipSpacePos.xyz / LightClipSpacePos.w;
    ProjCoords.xy = (ProjCoords.xy * 0.5f) + 0.5f;
    ProjCoords.y  = 1.0f - ProjCoords.y;
    
    return ProjCoords;
}

float3 GetLightViewPos(uint CascadeIndex, float3 World)
{
    float4 LightViewPosition = mul(float4(World, 1.0f), CascadeMatrixBuffer[CascadeIndex].View);
    return LightViewPosition.xyz;
}

float CascadedShadowFactor(float3 World, float3 View, float3 N, float3 L, inout uint RandomSeed)
{
    float ShadowBias = max(DirLightBuffer.MaxShadowBias * (1.0f - (max(dot(N, L), 0.0f))), DirLightBuffer.ShadowBias);
    
    float CascadeWeights[NUM_SHADOW_CASCADES];
    for (int i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        CascadeWeights[i] = 0.0f;
    }
    
    float CurrentNearPlane = 0.01f;
    for (i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        float CurrentSplit = CascadeSplitsBuffer[i].Split;
        if (View.z < CurrentSplit)
        {
#if BLEND_CASCADES
            float Range = View.z - CurrentNearPlane;
            float SplitRange = CurrentSplit - CurrentNearPlane;
        
            float Percentage = Range / SplitRange;
            if (Percentage >= (1.0f - BAND_PERCENTAGE))
            {
                float Weight = (1.0f - Percentage) / BAND_PERCENTAGE;
                CascadeWeights[i] = Weight;
                
                int NextIndex = min(i + 1, NUM_SHADOW_CASCADES - 1);
                if (NextIndex != i)
                {
                    CascadeWeights[NextIndex] = 1.0f - CascadeWeights[i];
                }
            }
            else
#endif
            {
                CascadeWeights[i] = 1.0f;
            }
            
            break;
        }
        
        CurrentNearPlane = CurrentSplit;
    }
    
    float LightSize = DirLightBuffer.LightSize;
    float Shadow = 0.0f;
    if (CascadeWeights[0] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(0, World);
        float3 LightViewPos = GetLightViewPos(0, World);
        float LightFar = CascadeSplitsBuffer[0].FarPlane;
        Shadow += PCSSDirectionalLight(ShadowCascade0, ProjCoords, LightViewPos, LightSize, 0.01f, LightFar, 1.0f / 8.0f, ShadowBias, RandomSeed) * CascadeWeights[0];
    }
    if (CascadeWeights[1] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(1, World);
        float3 LightViewPos = GetLightViewPos(1, World);
        float LightFar = CascadeSplitsBuffer[1].FarPlane;
        Shadow += PCSSDirectionalLight(ShadowCascade1, ProjCoords, LightViewPos, LightSize, 0.01f, LightFar, 1.0f / 4.0f, ShadowBias, RandomSeed) * CascadeWeights[1];
    }
    if (CascadeWeights[2] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(2, World);
        float3 LightViewPos = GetLightViewPos(2, World);
        float LightFar = CascadeSplitsBuffer[2].FarPlane;
        Shadow += PCSSDirectionalLight(ShadowCascade2, ProjCoords, LightViewPos, LightSize, 0.01f, LightFar, 1.0f / 2.0f, ShadowBias, RandomSeed) * CascadeWeights[2];
    }
    if (CascadeWeights[3] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(3, World);
        float3 LightViewPos = GetLightViewPos(3, World);
        float LightFar = CascadeSplitsBuffer[3].FarPlane;
        Shadow += PCSSDirectionalLight(ShadowCascade3, ProjCoords, LightViewPos, LightSize, 0.01f, LightFar, 1.0f, ShadowBias, RandomSeed) * CascadeWeights[3];
    }
    
    return Shadow;
}

// Tiled Light Culling
groupshared uint GroupMinZ;
groupshared uint GroupMaxZ;
groupshared uint GroupPointLightCounter;
groupshared uint GroupPointLightIndices[MAX_LIGHTS_PER_TILE];
groupshared uint GroupShadowPointLightCounter;
groupshared uint GroupShadowPointLightIndices[MAX_LIGHTS_PER_TILE];

float GetNumTilesX()
{
    return DivideByMultiple(ScreenWidth, THREAD_COUNT);
}

float GetNumTilesY()
{
    return DivideByMultiple(ScreenHeight, THREAD_COUNT);
}

[numthreads(THREAD_COUNT, THREAD_COUNT, 1)]
void Main(ComputeShaderInput Input)
{
    uint ThreadIndex = Input.GroupThreadID.y * THREAD_COUNT + Input.GroupThreadID.x;
    if (ThreadIndex == 0)
    {
        GroupMinZ = 0x7f7fffff;
        GroupMaxZ = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();

    uint2 TexCoord  = Input.DispatchThreadID.xy;
    float Depth     = DepthStencilTex.Load(int3(TexCoord, 0));
    float ViewPosZ  = Depth_ProjToView(Depth, CameraBuffer.ProjectionInverse);
    
    // TODO: If we change to reversed Z then we need to change from 1.0 to 0.0
    
    uint z = asuint(ViewPosZ);
    if (Depth < 1.0f)
    {
        InterlockedMin(GroupMinZ, z);
        InterlockedMax(GroupMaxZ, z);
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    float MinZ = asfloat(GroupMinZ);
    float MaxZ = asfloat(GroupMaxZ);
    
    float4 Frustum[4];
    {
        float pxm    = float(THREAD_COUNT * Input.GroupID.x);
        float pym    = float(THREAD_COUNT * Input.GroupID.y);
        float pxp    = float(THREAD_COUNT * (Input.GroupID.x + 1));
        float pyp    = float(THREAD_COUNT * (Input.GroupID.y + 1));
        float Width  = THREAD_COUNT * GetNumTilesX();
        float Height = THREAD_COUNT * GetNumTilesY();
        
        float3 CornerPoints[4];
        CornerPoints[0] = Float3_ProjToView(
            float3((pxm / Width) * 2.0f - 1.0f, ((Height - pym) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[1] = Float3_ProjToView(
            float3((pxp / Width) * 2.0f - 1.0f, ((Height - pym) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[2] = Float3_ProjToView(
            float3((pxp / Width) * 2.0f - 1.0f, ((Height - pyp) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);
        
        CornerPoints[3] = Float3_ProjToView(
            float3((pxm / Width) * 2.0f - 1.0f, ((Height - pyp) / Height) * 2.0f - 1.0f, 1.0f),
            CameraBuffer.ProjectionInverse);

        for (uint i = 0; i < 4; i++)
        {
            Frustum[i] = CreatePlane(CornerPoints[i], CornerPoints[(i + 1) & 3]);
        }
    }
    
    if (ThreadIndex == 0)
    {
        GroupPointLightCounter       = 0;
        GroupShadowPointLightCounter = 0;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    for (uint i = ThreadIndex; i < NumPointLights; i += TOTAL_THREAD_COUNT)
    {
        float3 Pos     = PointLightsPosRad[i].Position;
        float3 ViewPos = mul(float4(Pos, 1.0f), CameraBuffer.View).xyz;
        float  Radius  = PointLightsPosRad[i].Radius;

        if ((GetSignedDistanceFromPlane(ViewPos, Frustum[0]) < Radius) && 
            (GetSignedDistanceFromPlane(ViewPos, Frustum[1]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[2]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[3]) < Radius) &&
            (-ViewPos.z + MinZ < Radius) && (ViewPos.z - MaxZ < Radius))
        {
            uint Index = 0;
            InterlockedAdd(GroupPointLightCounter, 1, Index);
            GroupPointLightIndices[Index] = i;
        }
    }
    
    for (uint j = ThreadIndex; j < NumShadowCastingPointLights; j += TOTAL_THREAD_COUNT)
    {
        float3 Pos     = ShadowCastingPointLightsPosRad[j].Position;
        float3 ViewPos = mul(float4(Pos, 1.0f), CameraBuffer.View).xyz;
        float  Radius  = ShadowCastingPointLightsPosRad[j].Radius;

        if ((GetSignedDistanceFromPlane(ViewPos, Frustum[0]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[1]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[2]) < Radius) &&
            (GetSignedDistanceFromPlane(ViewPos, Frustum[3]) < Radius) &&
            (-ViewPos.z + MinZ < Radius) && (ViewPos.z - MaxZ < Radius))
        {
            uint Index = 0;
            InterlockedAdd(GroupShadowPointLightCounter, 1, Index);
            GroupShadowPointLightIndices[Index] = j;
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    const float2 TexCoordFloat   = float2(TexCoord) / float2(ScreenWidth, ScreenHeight);
    const float3 WorldPosition   = PositionFromDepth(Depth, TexCoordFloat, CameraBuffer.ViewProjectionInverse);
    const float3 ViewPosition    = PositionFromDepth(Depth, TexCoordFloat, CameraBuffer.ProjectionInverse);
    const float3 GBufferAlbedo   = AlbedoTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferMaterial = MaterialTex.Load(int3(TexCoord, 0)).rgb;
    const float3 GBufferNormal   = NormalTex.Load(int3(TexCoord, 0)).rgb;
    const float  ScreenSpaceAO   = SSAO.Load(int3(TexCoord, 0)).r;
    
    const float3 N = UnpackNormal(GBufferNormal);
    const float3 V = normalize(CameraBuffer.Position - WorldPosition);
    const float GBufferRoughness = GBufferMaterial.r;
    const float GBufferMetallic  = GBufferMaterial.g;
    const float GBufferAO        = GBufferMaterial.b * ScreenSpaceAO;
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, GBufferAlbedo, GBufferMetallic);
    
    float3 L0 = Float3(0.0f);
    
    // Pointlights
    for (uint i = 0; i < GroupPointLightCounter; i++)
    {
        int Index = GroupPointLightIndices[i];
        const PointLight     Light       = PointLights[Index];
        const PositionRadius LightPosRad = PointLightsPosRad[Index];

        float3 L = LightPosRad.Position - WorldPosition;
        float DistanceSqrd = dot(L, L);
        float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
        L = normalize(L);

        float3 IncidentRadiance = Light.Color * Attenuation;
        IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
        L0 += IncidentRadiance;
    }
    
    for (uint i = 0; i < GroupShadowPointLightCounter; i++)
    {
        int Index = GroupShadowPointLightIndices[i];
        const ShadowPointLight Light       = ShadowCastingPointLights[Index];
        const PositionRadius   LightPosRad = ShadowCastingPointLightsPosRad[Index];
     
        float ShadowFactor = PointLightShadowFactor(PointLightShadowMaps, float(Index), ShadowMapSampler0, WorldPosition, N, Light, LightPosRad);
        if (ShadowFactor > 0.001f)
        {
            float3 L = LightPosRad.Position - WorldPosition;
            float DistanceSqrd = dot(L, L);
            float Attenuation  = 1.0f / max(DistanceSqrd, 0.01f * 0.01f);
            L = normalize(L);
            
            float3 IncidentRadiance = Light.Color * Attenuation;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // DirectionalLights
    float CascadeWeights[NUM_SHADOW_CASCADES];
    float ShadowFactor;
    {
        const DirectionalLight Light = DirLightBuffer;
        float3 L = normalize(-Light.Direction);
        
        // NOTE: For debugging
        for (uint i = 0; i < NUM_SHADOW_CASCADES; i++)
        {
            CascadeWeights[i] = 0.0f;
        }
    
        float CurrentNearPlane = 0.01f;
        for (int i = 0; i < NUM_SHADOW_CASCADES; i++)
        {
            float CurrentSplit = CascadeSplitsBuffer[i].Split;
            if (ViewPosition.z < CurrentSplit)
            {
#if BLEND_CASCADES
                float Range = ViewPosition.z - CurrentNearPlane;
                float SplitRange = CurrentSplit - CurrentNearPlane;
        
                float Percentage = Range / SplitRange;
                if (Percentage >= (1.0f - BAND_PERCENTAGE))
                {
                    float Weight = (1.0f - Percentage) / BAND_PERCENTAGE;
                    CascadeWeights[i] = Weight;
                
                    int NextIndex = min(i + 1, NUM_SHADOW_CASCADES - 1);
                    if (NextIndex != i)
                    {
                        CascadeWeights[NextIndex] = 1.0f - CascadeWeights[i];
                    }
                }
                else
#endif
                {
                    CascadeWeights[i] = 1.0f;
                }
            
                break;
            }
        
            CurrentNearPlane = CurrentSplit;
        }
        
        uint RandomSeed = InitRandom(TexCoord, ScreenWidth, 0);
        
        ShadowFactor = CascadedShadowFactor(WorldPosition, ViewPosition, N, L, RandomSeed);
        if (ShadowFactor > 0.0f)
        {
            float3 IncidentRadiance = Light.Color;
            IncidentRadiance = DirectRadiance(F0, N, V, L, IncidentRadiance, GBufferAlbedo, GBufferRoughness, GBufferMetallic);
            
            L0 += IncidentRadiance * ShadowFactor;
        }
    }
    
    // Image Based Lightning
    float3 FinalColor = L0;
    {
        const float NDotV = max(dot(N, V), 0.0f);
        
        float3 F  = FresnelSchlick_Roughness(F0, V, N, GBufferRoughness);
        float3 Ks = F;
        float3 Kd = Float3(1.0f) - Ks;
        float3 Irradiance = IrradianceMap.SampleLevel(IrradianceSampler, N, 0.0f).rgb;
        float3 Diffuse    = Irradiance * GBufferAlbedo * Kd;

        float3 R = reflect(-V, N);
        float3 PrefilteredMap  = SpecularIrradianceMap.SampleLevel(IrradianceSampler, R, GBufferRoughness * (NumSkyLightMips - 1.0f)).rgb;
        float2 BRDFIntegration = IntegrationLUT.SampleLevel(LUTSampler, float2(NDotV, GBufferRoughness), 0.0f).rg;
        float3 Specular        = PrefilteredMap * (F * BRDFIntegration.x + BRDFIntegration.y);

        float3 Ambient = (Diffuse + Specular) * GBufferAO;
        FinalColor     = Ambient + L0;
    }

#if DRAW_TILE_OCCUPANCY
    const uint TotalLightCount = GroupPointLightCounter + GroupShadowPointLightCounter;
    
    float4 Tint = Float4(1.0f);
    if (TotalLightCount < 8)
    {
        float Col = float(TotalLightCount) / 8.0f;
        Tint = float4(0.0f, Col, 0.0f, 1.0f);
    }
    else if (TotalLightCount < 16)
    {
        float Col = float(TotalLightCount) / 16.0f;
        Tint = float4(0.0f, Col, Col, 1.0f);
    }
    else if (TotalLightCount < 32)
    {
        float Col = float(TotalLightCount) / 32.0f;
        Tint = float4(0.0f, 0.0f, Col, 1.0f);
    }
    else if (TotalLightCount < 64)
    {
        float Col = float(TotalLightCount) / 64.0f;
        Tint = float4(Col, Col, 0.0f, 1.0f);
    }
    else
    {
        float Col = float(TotalLightCount) / float(NumPointLights + NumShadowCastingPointLights);
        Tint = float4(Col, 0.0f, 0.0f, 1.0f);
    }
    
    FinalColor = FinalColor * Tint.rgb;
#elif DRAW_SHADOW_CASCADE
    float4 Tint = Float4(1.0f);
    Tint += float4(1.0f, 0.0f, 0.0f, 1.0f) * CascadeWeights[0];
    Tint += float4(0.0f, 1.0f, 0.0f, 1.0f) * CascadeWeights[1];
    Tint += float4(0.0f, 0.0f, 1.0f, 1.0f) * CascadeWeights[2];
    Tint += float4(1.0f, 1.0f, 0.0f, 1.0f) * CascadeWeights[3];
    
    FinalColor = FinalColor * Tint.rgb;
#endif
    
    // Finalize
    float FinalLuminance = Luminance(FinalColor);
    FinalColor = ApplyGammaCorrectionAndTonemapping(FinalColor);
    Output[TexCoord] = float4(FinalColor, FinalLuminance);
}