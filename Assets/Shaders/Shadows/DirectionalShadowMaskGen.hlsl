#include "../Structs.hlsli"
#include "../Random.hlsli"
#include "../Helpers.hlsli"
#include "../Poisson.hlsli"
#include "../Halton.hlsli"
#include "CascadeStructs.hlsli"

#ifndef NUM_THREADS
    #define NUM_THREADS 16
#endif

#ifndef ENABLE_DEBUG
    #define ENABLE_DEBUG 0
#endif

// Soft shadows settings
#ifndef FILTER_MODE_PCF_GRID
    #define FILTER_MODE_PCF_GRID 0
#endif

#ifndef FILTER_MODE_PCF_POISSION_DISC
    #define FILTER_MODE_PCF_POISSION_DISC 1
#endif

// Poisson Disc Settings
#ifndef NUM_PCF_SAMPLES
    #define NUM_PCF_SAMPLES 32
#endif

#ifndef ROTATE_SAMPLES
    #define ROTATE_SAMPLES 1
#endif

#ifndef NUM_BLOCKER_SAMPLES
    #define NUM_BLOCKER_SAMPLES 16
#endif

// Cascaded Shadow Mapping Settings
#ifndef SELECT_CASCADE_FROM_PROJECTION
    #define SELECT_CASCADE_FROM_PROJECTION 1
#endif

#ifndef BLEND_CASCADES
    #define BLEND_CASCADES 1
#endif

#ifndef CASCADE_FADE_FACTOR
    #define CASCADE_FADE_FACTOR 0.1
#endif

#define ENABLE_PCSS 1
#define ENABLE_FRAME_INDEX 0

#define USE_ORTHO 1

// Camera and Light
#if SHADER_LANG == SHADER_LANG_MSL
ConstantBuffer<FCamera>           CameraBuffer : register(b2);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b3);
#else
ConstantBuffer<FCamera>           CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);
#endif

struct FDirectionalShadowSettings
{
    float FilterSize;
    float MaxFilterSize;
    uint  ShadowMapSize;
    uint  FrameIndex;
};

ConstantBuffer<FDirectionalShadowSettings> SettingsBuffer : register(b2);

// Shadow information
StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<FCascadeSplit>    ShadowSplitsBuffer   : register(t1);

// G-Buffer
Texture2D<float>  DepthBuffer  : register(t2);
Texture2D<float3> NormalBuffer : register(t3);

// Shadow Cascades
Texture2DArray<float> ShadowCascades : register(t4);

// Output
RWTexture2D<float> Output : register(u0);
#if ENABLE_DEBUG
RWTexture2D<uint> CascadeIndexTex : register(u1);
#endif

// Samplers
SamplerComparisonState ShadowSamplerPointCmp  : register(s0);
SamplerComparisonState ShadowSamplerLinearCmp : register(s1);

SamplerState ShadowSamplerPoint : register(s2);

float2 GetPoissonSample(uint Index)
{
#if (NUM_PCF_SAMPLES == 16)
    return PoissonDisk16[Index];
#elif (NUM_PCF_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_PCF_SAMPLES == 64)
    return PoissonDisk64[Index];
#elif (NUM_PCF_SAMPLES == 128)
    return PoissonDisk128[Index];
#else
    return 0.0;
#endif
}

float2 GetBlockerPoissonSample(uint Index)
{
#if (NUM_BLOCKER_SAMPLES == 16)
    return PoissonDisk16[Index];
#elif (NUM_BLOCKER_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_BLOCKER_SAMPLES == 64)
    return PoissonDisk64[Index];
#elif (NUM_BLOCKER_SAMPLES == 128)
    return PoissonDisk128[Index];
#else
    return 0.0;
#endif
}

float GetShadowMapSize()
{
    return (float)SettingsBuffer.ShadowMapSize;
}

struct FFilterSetup
{
    float3 WorldPosition;
    float3 Normal; 
    float2 ShadowPosition;

    float  BiasedDepth;

#if ROTATE_SAMPLES
    float2x2 SampleRotationMatrix;
#endif
};

float2 ComputeBlockerDepth(uint CascadeIndex, FFilterSetup FilterSetup, float SearchSize)
{
    float NumBlockers     = 0.0;
    float BlockerDepthSum = 0.0;

    // Calculate the size of the filter
    const float2 FilterRadius = SearchSize.xx * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy) * 0.5;

    // Use Poisson sampling for the blocker search
    for (int Sample = 0; Sample < NUM_BLOCKER_SAMPLES; ++Sample)
    {
        float2 SampleOffset = GetBlockerPoissonSample(Sample) * FilterRadius;

    #if ROTATE_SAMPLES
        SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
    #endif

        // Accumulate blockers
        float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(FilterSetup.ShadowPosition + SampleOffset, CascadeIndex), 0);
        if (SampleDepth < FilterSetup.BiasedDepth)
        {
            BlockerDepthSum += SampleDepth;
            NumBlockers += 1.0;
        }
    }

    // Return the average blocker depth
    return float2(BlockerDepthSum / NumBlockers, NumBlockers);
}

float ShadowAmountPoissonDisc(uint CascadeIndex, FFilterSetup FilterSetup, float PenumbraSize)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
    #if ENABLE_PCSS
        const float2 FilterRadius = PenumbraSize.xx * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy) * 0.5;
    #else
        const float  ShadowMapSize = GetShadowMapSize();
        const float2 FilterRadius  = (FilterSize * 0.5) / ShadowMapSize;
    #endif

        for (int Sample = 0; Sample < NUM_PCF_SAMPLES; ++Sample)
        {
            float2 SampleOffset = GetPoissonSample(Sample) * FilterRadius;
        
        #if ROTATE_SAMPLES
            SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
        #endif

            Result += ShadowCascades.SampleCmpLevelZero(ShadowSamplerPointCmp, float3(FilterSetup.ShadowPosition + SampleOffset, CascadeIndex), FilterSetup.BiasedDepth);
        }

        Result = Result / float(NUM_PCF_SAMPLES);
    }
    else
    {
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinearCmp, float3(FilterSetup.ShadowPosition, CascadeIndex), FilterSetup.BiasedDepth);
    }

    return saturate(Result);
}

float ShadowAmountGrid(uint CascadeIndex, FFilterSetup FilterSetup, float PenumbraSize)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx * PenumbraSize, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float ShadowMapSize = GetShadowMapSize();
        const float TexelSize     = 1.0 / ShadowMapSize;

        const float2 ShadowTexel   = FilterSetup.ShadowPosition * ShadowMapSize;
        const float2 TexelFraction = frac(ShadowTexel);
        const float2 FilterRadius  = FilterSize / 2.0;

        int2 MinOffset = int2(floor(TexelFraction - FilterRadius));
        int2 MaxOffset = int2(TexelFraction + FilterRadius);

        [loop]
        for (int SampleY = MinOffset.y; SampleY <= MaxOffset.y; ++SampleY)
        {
            float WeightY = 1.0;
            if(SampleY == MinOffset.y)
            {
                WeightY = saturate((FilterRadius.y - TexelFraction.y) + 1.0 + SampleY);
            }
            else if(SampleY == MaxOffset.y)
            {
                WeightY = saturate(FilterRadius.y + TexelFraction.y - SampleY);
            }

            [loop]
            for (int SampleX = MinOffset.x; SampleX <= MaxOffset.x; ++SampleX)
            {
                const float2 SampleOffset     = float2(SampleX, SampleY) * TexelSize;
                const float2 CurrentTexCoords = FilterSetup.ShadowPosition + SampleOffset;

                float WeightX = 1.0;
                if(SampleX == MinOffset.x)
                {
                    WeightX = saturate((FilterRadius.x - TexelFraction.x) + 1.0 + SampleX);
                }
                else if(SampleX == MaxOffset.x)
                {
                    WeightX = saturate(FilterRadius.x + TexelFraction.x - SampleX);
                }

                const float Weight = WeightX * WeightY;
                const float Sample = ShadowCascades.SampleCmpLevelZero(ShadowSamplerPointCmp, float3(CurrentTexCoords, CascadeIndex), FilterSetup.BiasedDepth);
                Result += Sample * Weight;
            }
        }

        const float NumSamples = FilterSize.x * FilterSize.y;
        Result = Result / NumSamples;
    }
    else
    {
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinearCmp, float3(FilterSetup.ShadowPosition, CascadeIndex), FilterSetup.BiasedDepth);
    }

    return saturate(Result);
}

float ShadowAmountSimple(uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float Sample = ShadowCascades.SampleCmpLevelZero(ShadowSamplerPointCmp, float3(FilterSetup.ShadowPosition, CascadeIndex), FilterSetup.BiasedDepth);
    return saturate(Sample);
}

float PCSS_SearchRadiusUV(float DepthVS, float CascadeNearPlane)
{
    const float LightRadiusUV = LightBuffer.LightSize;
	return LightRadiusUV * (DepthVS - CascadeNearPlane) / DepthVS;
}

float PCSS_PenumbraRadiusUV(float RecieverDepthVS, float BlockerDepthVS)
{
    return abs(RecieverDepthVS - BlockerDepthVS) / BlockerDepthVS;
}

float PCSS_ProjectToLightUV(float PenumbraRadiusUV, float DepthVS, float CascadeNearPlane)
{
    const float LightRadiusUV = LightBuffer.LightSize;
	return LightRadiusUV * PenumbraRadiusUV * CascadeNearPlane / DepthVS;
}

float PCSS_ClipToEye(float DepthVS, float CascadeNearPlane, float CascadeFarPlane)
{
#if USE_ORTHO
    return CascadeNearPlane + (CascadeFarPlane - CascadeNearPlane) * DepthVS;
#else
	return CascadeFarPlane * CascadeNearPlane / (CascadeFarPlane - DepthVS * (CascadeFarPlane - CascadeNearPlane));
#endif
}

float CascadeShadowAmount(uint CascadeIndex, float3 PositionWS, float3 NormalWS, float3 ShadowPosition, inout uint RandomSeed)
{
    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];
    ShadowPosition += CascadeSplit.Offsets.xyz;
    ShadowPosition *= CascadeSplit.Scale.xyz;

    // Calculate Biased Depth
    const float NDotL       = saturate(dot(NormalWS, LightBuffer.Direction)); 
    const float BiasScale   = (1.0 - NDotL);
    const float ShadowBias  = min(max(0.0001, LightBuffer.ShadowBias), BiasScale * max(0.0005, LightBuffer.MaxShadowBias));
    const float BiasedDepth = ShadowPosition.z - ShadowBias;

    FFilterSetup FilterSetup;
    FilterSetup.WorldPosition  = PositionWS;
    FilterSetup.Normal         = NormalWS;
    FilterSetup.ShadowPosition = ShadowPosition.xy;
    FilterSetup.BiasedDepth    = BiasedDepth;

#if ROTATE_SAMPLES
    float Theta    = NextRandom(RandomSeed) * PI_2;
    float CosTheta = cos(Theta);
    float SinTheta = sin(Theta);
    FilterSetup.SampleRotationMatrix = float2x2(float2(CosTheta, -SinTheta), float2(SinTheta,  CosTheta));
#endif

#if ENABLE_PCSS
    // PCSS Step 1: With PCSS we need to calculate the number of blockers and the average depth
    FCascadeMatrices CascadeMatrices = ShadowMatricesBuffer[CascadeIndex];

    float4 PositionVS = mul(float4(PositionWS, 1.0), CascadeMatrices.View);
    PositionVS.xyz /= PositionVS.w;

    // Calculate the blocker search radius
    const float DepthVS             = -PositionVS.z;
    const float BlockerSearchSizeUV = PCSS_SearchRadiusUV(DepthVS, CascadeSplit.NearPlane);
    
    // In case we did not find any blockers, then we can just stop here
    const float2 BlockerInfo = ComputeBlockerDepth(CascadeIndex, FilterSetup, BlockerSearchSizeUV);
    if (BlockerInfo.y < 1.0)
    {
        return 1.0;
    }

    // PCSS Step 2: Penumbra size
    const float AvgBlockerDepthVS = BlockerInfo.x;
    const float BlockerDepthVS    = PCSS_ClipToEye(AvgBlockerDepthVS, CascadeSplit.NearPlane, CascadeSplit.FarPlane);
    const float PenumbraWidth     = PCSS_PenumbraRadiusUV(DepthVS, BlockerDepthVS);
    const float PenumbraRadius    = PCSS_ProjectToLightUV(PenumbraWidth, DepthVS, CascadeSplit.NearPlane);
#else
    const float PenumbraRadius = 1.0;
#endif

    // PCSS Step 3: Filter the shadows
#if FILTER_MODE_PCF_GRID
    return ShadowAmountGrid(CascadeIndex, FilterSetup, PenumbraRadius);
#elif FILTER_MODE_PCF_POISSION_DISC
    return ShadowAmountPoissonDisc(CascadeIndex, FilterSetup, PenumbraRadius);
#else
    return ShadowAmountSimple(CascadeIndex, FilterSetup);
#endif
}

float ComputeShadow(float3 PositionWS, float3 Normal, float DepthVS, inout uint CascadeIndex, inout uint RandomSeed)
{
    // Calculate z-position in view-space
    const float  ViewPosZ           = Depth_ProjToView(DepthVS, CameraBuffer.ProjectionInv);
    const float3 ProjectionPosition = mul(float4(PositionWS, 1.0), LightBuffer.ShadowMatrix).xyz;

    // Find current cascade
    CascadeIndex = NUM_SHADOW_CASCADES - 1;

    [unroll]
    for (int Index = NUM_SHADOW_CASCADES - 1; Index >= 0; --Index)
    {
        FCascadeSplit CascadeSplit = ShadowSplitsBuffer[Index];

    #if SELECT_CASCADE_FROM_PROJECTION
        const float4 Offsets = CascadeSplit.Offsets;
        const float4 Scale   = CascadeSplit.Scale;

        float3 CascadePosition = ProjectionPosition + Offsets.xyz;
        CascadePosition *= Scale.xyz;
        CascadePosition  = abs(CascadePosition - 0.5);

        if (all(CascadePosition <= 0.5))
        {
            CascadeIndex = Index;
        }
    #else
        const float CurrentSplit = CascadeSplit.Split;
        if (ViewPosZ < CurrentSplit)
        {
            CascadeIndex = Index;
        }
    #endif
    }

    // Calculate shadow factor
    float ShadowAmount = CascadeShadowAmount(CascadeIndex, PositionWS, Normal, ProjectionPosition, RandomSeed);

    // Blend between this and next cascade
    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];

#if BLEND_CASCADES
    float NextSplit  = CascadeSplit.Split;
    float SplitSize  = (CascadeIndex == 0) ? NextSplit : (NextSplit - ShadowSplitsBuffer[CascadeIndex - 1].Split);
    float FadeFactor = (NextSplit - ViewPosZ) / SplitSize;
  
#if SELECT_CASCADE_FROM_PROJECTION
    const float4 Offsets = CascadeSplit.Offsets;
    const float4 Scale   = CascadeSplit.Scale;

    float3 CascadePosition = ProjectionPosition + Offsets.xyz;
    CascadePosition *= Scale.xyz;
    CascadePosition  = abs(CascadePosition * 2.0 - 1.0);

    float DistToEdge = 1.0 - max(max(CascadePosition.x, CascadePosition.y), CascadePosition.z);
    FadeFactor = max(DistToEdge, FadeFactor);
#endif

    [branch]
    if(FadeFactor <= CASCADE_FADE_FACTOR && CascadeIndex != (NUM_SHADOW_CASCADES - 1))
    {
        const float NextSplitVisibility = CascadeShadowAmount(CascadeIndex + 1, PositionWS, Normal, ProjectionPosition, RandomSeed);
        const float LerpAmount = smoothstep(0.0, CASCADE_FADE_FACTOR, FadeFactor);
        ShadowAmount = lerp(NextSplitVisibility, ShadowAmount, LerpAmount);
    }
#endif

    return ShadowAmount;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const uint2 Pixel = Input.DispatchThreadID.xy;
   
    // Discard pixels not rendered to the GBuffer
    float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0));
    if (dot(GBufferNormal, GBufferNormal) == 0)
    {
        Output[Pixel] = 1.0;
        return;
    }

    const float  Depth         = DepthBuffer.Load(int3(Pixel, 0)); 
    const float2 PixelCenter   = float2(Pixel) + 0.5;
    const float2 TexCoord      = PixelCenter / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 Normal        = UnpackNormal(GBufferNormal);

    // Random Seed when doing soft shadows
#if ENABLE_FRAME_INDEX
    const uint FrameIndex = SettingsBuffer.FrameIndex;
#else
    const uint FrameIndex = 0;
#endif

    // Initialize random-seed
    uint RandomSeed = InitRandom(Pixel, CameraBuffer.ViewportWidth, FrameIndex);
    
    // Cascade-index is written to for debug purposes
    uint CascadeIndex = 0;

    // Calculate the Shadow
    const float ShadowAmount = ComputeShadow(WorldPosition, Normal, Depth, CascadeIndex, RandomSeed);
    Output[Pixel] = ShadowAmount;

    // Output debug-information needed when visualizing the cascades
#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}