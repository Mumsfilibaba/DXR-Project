#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

#define NUM_THREADS (16)

#ifndef ENABLE_DEBUG
    #define ENABLE_DEBUG (0)
#endif

// FCamera and Light
ConstantBuffer<FCamera>           CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);

// Shadow information
StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<FCascadeSplit>    ShadowSplitsBuffer   : register(t1);

// G-Buffer
Texture2D<float3> NormalTex : register(t2);
Texture2D<float>  DepthTex  : register(t3);

// Shadow Cascades
Texture2D<float> ShadowCascade0 : register(t4);
Texture2D<float> ShadowCascade1 : register(t5);
Texture2D<float> ShadowCascade2 : register(t6);
Texture2D<float> ShadowCascade3 : register(t7);

// Output
RWTexture2D<float> Output : register(u0);

#if ENABLE_DEBUG
RWTexture2D<uint> CascadeIndexTex : register(u1);
#endif

// Samplers
SamplerState Sampler : register(s0);

// Soft shadows settings
#define NUM_BLOCKER_SAMPLES (32)
#define NUM_PCF_SAMPLES     (32)

// Cascaded Shadow Mapping
#define ENABLE_PCSS    (0)
#define BLEND_CASCADES (0)
#define CASCADE_FADING (0.1f)
#define PCF_RADIUS     (0.001f)
#define ROTATE_SAMPLES (1)

 /** @brief: Shadow Helpers */

float2 SamplePoissonBlocker(uint Index)
{
#if (NUM_BLOCKER_SAMPLES == 16)
    return PoissonDisk16[Indexi];
#elif (NUM_BLOCKER_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_BLOCKER_SAMPLES == 64)
    return PoissonDisk64[Index];
#elif (NUM_BLOCKER_SAMPLES == 128)
    return PoissonDisk128[Index];
#endif
}

float2 SamplePoissonPCF(uint Index)
{
#if (NUM_PCF_SAMPLES == 16)
    return PoissonDisk16[Index];
#elif (NUM_PCF_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_BLOCKER_SAMPLES == 64)
    return PoissonDisk64[Index];
#elif (NUM_PCF_SAMPLES == 128)
    return PoissonDisk128[Index];
#endif
}

float2 RotatePoisson(float2 Sample, inout uint RandomSeed)
{
    return OneToMinusOne(CranleyPatterssonRotation(MinusOneToOne(Sample), RandomSeed));
}

float3 GetShadowCoords(uint CascadeIndex, float3 World)
{
    float4 LightClipSpacePos = mul(float4(World, 1.0f), ShadowMatricesBuffer[CascadeIndex].ViewProj);

    float3 ProjCoords = LightClipSpacePos.xyz / LightClipSpacePos.w;
    ProjCoords.xy = (ProjCoords.xy * 0.5f) + 0.5f;
    ProjCoords.y = 1.0f - ProjCoords.y;

    return ProjCoords;
}

float3 GetLightViewPos(uint CascadeIndex, float3 World)
{
    return mul(float4(World, 1.0f), ShadowMatricesBuffer[CascadeIndex].View).xyz;
}

// Based on: http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float LightSize, float LightNear, float ReciverZ)
{
    return (LightSize * (ReciverZ - LightNear) / ReciverZ);
}

float FindBlockerDistance(in Texture2D<float> ShadowMap, float2 TexCoords, float CompareDepth, float SearchRadius)
{
    float AvgBlockerDistance = 0;
    int   NumBlockers = 0;
    for (int Index = 0; Index < NUM_BLOCKER_SAMPLES; Index++)
    {
        float2 RandomDirection = SamplePoissonBlocker(Index);
        RandomDirection = RandomDirection * SearchRadius;
        
        float Depth = ShadowMap.SampleLevel(Sampler, TexCoords.xy + RandomDirection, 0.0f);
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

float PCFDirectionalLight(in Texture2D<float> ShadowMap, float2 TexCoords, float CompareDepth, float PenumbraRadius, float Scale, inout uint RandomSeed)
{
    uint Width;
    uint Height;
    ShadowMap.GetDimensions(Width, Height);
    
    float Shadow = 0;
    for (int i = 0; i < NUM_PCF_SAMPLES; i++)
    {
        float2 RandomDirection = SamplePoissonPCF(i);
#if ROTATE_SAMPLES
        RandomDirection = RotatePoisson(RandomDirection, RandomSeed);
#endif
        RandomDirection *= PenumbraRadius;
        RandomDirection *= Scale;
        
        float Depth = ShadowMap.SampleLevel(Sampler, TexCoords.xy + RandomDirection, 0.0f);
        if (Depth < CompareDepth)
        {
            Shadow += 1.0f;
        }
    }
    
    return 1.0f - saturate(Shadow / float(NUM_PCF_SAMPLES));
}

float PCSSDirectionalLight(in Texture2D<float> ShadowMap, uint CascadeIndex, float3 WorldPosition, float Bias, inout uint RandomSeed)
{
    const float3 ShadowCoords = GetShadowCoords(CascadeIndex, WorldPosition);
    // TODO: Correct Sampler instead
    if (ShadowCoords.x > 1.0f || ShadowCoords.y > 1.0f || ShadowCoords.z > 1.0f || ShadowCoords.x < 0.0f || ShadowCoords.y < 0.0f || ShadowCoords.z < 0.0f)
    {
        return 1.0f;
    }
    
    const float LightNear = 0.0f;
    const float LightFar  = ShadowSplitsBuffer[CascadeIndex].FarPlane;
    // Size of the light in [0, 1]
    float LightSize = LightBuffer.LightSize;
    // Why 0.5f?
    const float NEAR = 0.5f;
    // Biased depth 
    const float CompareDepth = ShadowCoords.z - Bias;
    // Scale the PCF filter based on what cascade is used
    const float Scale = 1.0f / float(CascadeIndex + 1);

#if ENABLE_PCSS
    float BlockerSearchRadius = SearchWidth(LightSize, 0.0f, ShadowCoords.z);
    BlockerSearchRadius *= Scale;

    float BlockerDistance = FindBlockerDistance(ShadowMap, ShadowCoords.xy, CompareDepth, BlockerSearchRadius);
    if (BlockerDistance == -1.0f)
    {
        return 1.0f;
    }

    float PenumbraWidth  = (ShadowCoords.z - BlockerDistance) / BlockerDistance;
    float PenumbraRadius = (PenumbraWidth * LightSize * 0.1f);
    //PenumbraRadius = min(PenumbraRadius, 0.01f);

    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, PenumbraRadius, Scale, RandomSeed);
#else
    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, PCF_RADIUS, Scale, RandomSeed);
#endif
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const uint2 Pixel = Input.DispatchThreadID.xy;
   
    // Discard pixels not rendered to the GBuffer
    float3 GBufferNormal = NormalTex.Load(int3(Pixel, 0)).rgb;
    if (length(GBufferNormal) == 0)
    {
        Output[Pixel] = 1.0f;
        return;
    }

    const float2 TexCoord      = (float2(Pixel) + Float2(0.5f)) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float  Depth         = DepthTex.Load(int3(Pixel, 0)); 
    const float  ViewPosZ      = Depth_ProjToView(Depth, CameraBuffer.ProjectionInverse);
    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInverse);

    // Calcualte shadow bias based on the normal and light directions 
    float3 N = UnpackNormal(GBufferNormal);
    float3 L = normalize(-LightBuffer.Direction);

    float ShadowBias = max(LightBuffer.MaxShadowBias * (1.0f - (max(dot(N, L), 0.0f))), LightBuffer.ShadowBias) + 0.0001f;
    
    // Calculate the current cascade
    uint CascadeIndex = max(NUM_SHADOW_CASCADES - 1, 0);
    for (int Index = 0; Index < NUM_SHADOW_CASCADES; ++Index)
    {
        const float CurrentSplit = ShadowSplitsBuffer[Index].Split;
        if (ViewPosZ < CurrentSplit)
        {
            CascadeIndex = Index;
            break;
        }
    }
    
    // Random Seed when doing soft shadows 
    uint RandomSeed = InitRandom(Pixel, CameraBuffer.ViewportWidth, 0);    
    // Calculate shadow factor
    float ShadowAmount = 0.0f;

#if BLEND_CASCADES
    float Cascade0 = smoothstep(ShadowSplitsBuffer[0].Split + (CASCADE_FADING * 0.5f), ShadowSplitsBuffer[0].Split - (CASCADE_FADING * 0.5f), ViewPosZ);
    float Cascade1 = smoothstep(ShadowSplitsBuffer[1].Split + (CASCADE_FADING * 0.5f), ShadowSplitsBuffer[1].Split - (CASCADE_FADING * 0.5f), ViewPosZ);
    float Cascade2 = smoothstep(ShadowSplitsBuffer[2].Split + (CASCADE_FADING * 0.5f), ShadowSplitsBuffer[2].Split - (CASCADE_FADING * 0.5f), ViewPosZ);

    if (Cascade0 > 0.0f && Cascade0 < 1.0f)
    {
        float ShadowAmount0 = PCSSDirectionalLight(ShadowCascade0, 0, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount1 = PCSSDirectionalLight(ShadowCascade1, 1, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount0, ShadowAmount1, Cascade0);
    }
    else if (Cascade1 > 0.0f && Cascade1 < 1.0f)
    {
        float ShadowAmount1 = PCSSDirectionalLight(ShadowCascade1, 1, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount2 = PCSSDirectionalLight(ShadowCascade2, 2, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount1, ShadowAmount2, Cascade1);
    }
    else if (Cascade2 > 0.0f && Cascade2 < 1.0f)
    {
        float ShadowAmount2 = PCSSDirectionalLight(ShadowCascade2, 2, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount3 = PCSSDirectionalLight(ShadowCascade3, 3, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount2, ShadowAmount3, Cascade2);
    }
    else
#endif
    {
        // TODO: Use a texture array for cascades
        if (CascadeIndex == 0)
        {
            ShadowAmount = PCSSDirectionalLight(ShadowCascade0, 0, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 1)
        {
            ShadowAmount = PCSSDirectionalLight(ShadowCascade1, 1, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 2)
        {
            ShadowAmount = PCSSDirectionalLight(ShadowCascade2, 2, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 3)
        {
            ShadowAmount = PCSSDirectionalLight(ShadowCascade3, 3, WorldPosition, ShadowBias, RandomSeed);
        }
    }
    
    Output[Pixel] = ShadowAmount;

#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}