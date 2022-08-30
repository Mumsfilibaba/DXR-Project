#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

#define NUM_THREADS (16)

#ifndef ENABLE_DEBUG
    #define ENABLE_DEBUG (0)
#endif

#define SELECT_CASCADE_FROM_PROJECTION (1)

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
#define PCF_RADIUS     (0.06f)
#define ROTATE_SAMPLES (0)

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
    ProjCoords.y  = 1.0f - ProjCoords.y;
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

float SampleCascade(uint CascadeIndex, float2 TexCoords)
{
    if (CascadeIndex == 0)
    {
        return ShadowCascade0.SampleLevel(Sampler, TexCoords, 0.0f);
    }
    else if (CascadeIndex == 1)
    {
        return ShadowCascade1.SampleLevel(Sampler, TexCoords, 0.0f);
    }
    else if (CascadeIndex == 2)
    {
        return ShadowCascade2.SampleLevel(Sampler, TexCoords, 0.0f);
    }
    else if (CascadeIndex == 3)
    {
        return ShadowCascade3.SampleLevel(Sampler, TexCoords, 0.0f);
    }
    else
    {
        return 1.0f;
    }
}

// NOTE: This assumes that the cascades has the same dimensions
uint2 GetCascadeDimensions()
{
    uint2 CascadeSize;
    ShadowCascade0.GetDimensions(CascadeSize.x, CascadeSize.y);
    return CascadeSize;
}

float FindBlockerDistance(uint CascadeIndex, float2 TexCoords, float BiasedDepth, float SearchRadius)
{
    float AvgBlockerDistance = 0;
    
    int NumBlockers = 0;
    for (int Index = 0; Index < NUM_BLOCKER_SAMPLES; Index++)
    {
        float2 RandomDirection = SamplePoissonBlocker(Index);
        RandomDirection = RandomDirection * SearchRadius;
        
        float Depth = SampleCascade(CascadeIndex, TexCoords.xy + RandomDirection);
        if (Depth < BiasedDepth)
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

float PCFDirectionalLight(uint CascadeIndex, float2 TexCoords, float BiasedDepth, float PenumbraRadius, inout uint RandomSeed)
{
    float Shadow = 0;
    for (int Sample = 0; Sample < NUM_PCF_SAMPLES; ++Sample)
    {
        float2 RandomDirection = SamplePoissonPCF(Sample);
#if ROTATE_SAMPLES
        RandomDirection = RotatePoisson(RandomDirection, RandomSeed);
#endif
        RandomDirection *= PenumbraRadius;
        
        float Depth = SampleCascade(CascadeIndex, TexCoords.xy + RandomDirection);
        if (Depth < BiasedDepth)
        {
            Shadow += 1.0f;
        }
    }
    
    return 1.0f - saturate(Shadow / float(NUM_PCF_SAMPLES));
}

float PCSSDirectionalLight(uint CascadeIndex, float3 WorldPosition, float Bias, inout uint RandomSeed)
{
    const float3 ShadowCoords = GetShadowCoords(CascadeIndex, WorldPosition);   
    const float  RadiusScale = ShadowSplitsBuffer[CascadeIndex].Scale.x;
    const float  LightNear   = 0.01f;
    // Size of the light in [0, 1]
    const float  LightSize   = LightBuffer.LightSize;
    // Biased depth 
    const float  BiasedDepth = (ShadowCoords.z - Bias);

#if ENABLE_PCSS
    float BlockerSearchRadius = SearchWidth(LightSize, LightNear, ShadowCoords.z) * RadiusScale;
    float BlockerDistance     = FindBlockerDistance(CascadeIndex, ShadowCoords.xy, BiasedDepth, BlockerSearchRadius);
    if (BlockerDistance == -1.0f)
    {
        return 1.0f;
    }

    float PenumbraWidth  = (ShadowCoords.z - BlockerDistance) / BlockerDistance;
    float PenumbraRadius = (PenumbraWidth * LightSize);
    return PCFDirectionalLight(CascadeIndex, ShadowCoords.xy, BiasedDepth, PenumbraRadius, RandomSeed);
#else
    const float Radius = (PCF_RADIUS * RadiusScale);
    return PCFDirectionalLight(CascadeIndex, ShadowCoords.xy, BiasedDepth, Radius, RandomSeed);
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
#if SELECT_CASCADE_FROM_PROJECTION
    const float3 ProjectionPosition = mul(float4(WorldPosition, 1.0f), LightBuffer.ShadowMatrix).xyz;
#endif

    // Calcualte shadow bias based on the normal and light directions 
    float3 N = UnpackNormal(GBufferNormal);
    float3 L = normalize(-LightBuffer.Direction);

    float ShadowBias = max(LightBuffer.MaxShadowBias * (1.0f - (max(dot(N, L), 0.0f))), LightBuffer.ShadowBias) + 0.0001f;
    
    // Calculate the current cascade
    uint CascadeIndex = max(NUM_SHADOW_CASCADES - 1, 0);
    for (int Index = CascadeIndex; Index >= 0; --Index)
    {
#if SELECT_CASCADE_FROM_PROJECTION
        const float4 Offsets = ShadowSplitsBuffer[Index].Offsets;
        const float4 Scale   = ShadowSplitsBuffer[Index].Scale;

        float3 CascadePosition = ProjectionPosition + Offsets.xyz;
        CascadePosition *= Scale.xyz;
        CascadePosition  = abs(CascadePosition - 0.5f);
        // TODO: Fix this
        if(all(CascadePosition <= 0.45f))
        {
            CascadeIndex = Index;
        }
#else
        const float CurrentSplit = ShadowSplitsBuffer[Index].Split;
        if (ViewPosZ < CurrentSplit)
        {
            CascadeIndex = Index;
        }
#endif
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
        float ShadowAmount0 = PCSSDirectionalLight(0, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount1 = PCSSDirectionalLight(1, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount0, ShadowAmount1, Cascade0);
    }
    else if (Cascade1 > 0.0f && Cascade1 < 1.0f)
    {
        float ShadowAmount1 = PCSSDirectionalLight(1, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount2 = PCSSDirectionalLight(2, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount1, ShadowAmount2, Cascade1);
    }
    else if (Cascade2 > 0.0f && Cascade2 < 1.0f)
    {
        float ShadowAmount2 = PCSSDirectionalLight(2, WorldPosition, ShadowBias, RandomSeed);
        float ShadowAmount3 = PCSSDirectionalLight(3, WorldPosition, ShadowBias, RandomSeed);
        ShadowAmount = lerp(ShadowAmount2, ShadowAmount3, Cascade2);
    }
    else
#endif
    {
        // TODO: Use a texture array for cascades
        if (CascadeIndex == 0)
        {
            ShadowAmount = PCSSDirectionalLight(0, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 1)
        {
            ShadowAmount = PCSSDirectionalLight(1, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 2)
        {
            ShadowAmount = PCSSDirectionalLight(2, WorldPosition, ShadowBias, RandomSeed);
        }
        else if (CascadeIndex == 3)
        {
            ShadowAmount = PCSSDirectionalLight(3, WorldPosition, ShadowBias, RandomSeed);
        }
    }
    
    Output[Pixel] = ShadowAmount;

#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}