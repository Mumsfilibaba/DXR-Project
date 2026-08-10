#ifndef CASCADE_SHADOW_SAMPLING_HLSLI
#define CASCADE_SHADOW_SAMPLING_HLSLI

// ------------------------------------------------------------------------------------------------
//  The one place that knows how a cascaded shadow map is selected, biased and filtered.
//  A pass says where its cascade resources live and includes this:
//      #define CASCADE_SHADOW_SPLITS_REGISTER             t1
//      #define CASCADE_SHADOW_CASCADES_REGISTER           t4
//      #define CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER  s0
//      #define CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER s1
//      #define CASCADE_SHADOW_SAMPLER_POINT_REGISTER      s2
//      #include "Shadows/CascadeShadowSampling.hlsli"
//
//  These are register tokens rather than numbers, because unlike the material slots there is no
//  run of consecutive registers for the header to name.
//  The light and the filter tuning arrive as an FCascadeShadowContext rather than through constant
//  buffers of an agreed name, so a caller keeps them wherever it already has them: the shadow mask
//  reads a dedicated buffer, the forward pass folds them into its root constants.
//  Optional knobs, all shared with FShadowMaskCS's permutation so both callers can be compiled the
//  same way: SHADOW_FILTER_FUNCTION, SHADOW_FILTER_MODE, NUM_SAMPLES, NUM_BLOCKER_SAMPLES,
//  ROTATE_SAMPLES, SELECT_CASCADE_FROM_PROJECTION, ENABLE_CASCADE_BLENDING, CASCADE_FADE_FACTOR,
//  ENABLE_FIRST_CASCADE_ONLY and USE_ORTHO.
// ------------------------------------------------------------------------------------------------

#include "Constants.hlsli"
#include "Random.hlsli"
#include "PoissonDisk.hlsli"
#include "VogelDisk.hlsli"
#include "CascadeStructs.hlsli"

#ifndef CASCADE_SHADOW_SPLITS_REGISTER
    #error "CascadeShadowSampling.hlsli needs CASCADE_SHADOW_SPLITS_REGISTER to place the cascade splits buffer"
#endif

#ifndef CASCADE_SHADOW_CASCADES_REGISTER
    #error "CascadeShadowSampling.hlsli needs CASCADE_SHADOW_CASCADES_REGISTER to place the cascade depth array"
#endif

#ifndef CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER
    #error "CascadeShadowSampling.hlsli needs CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER"
#endif

#ifndef CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER
    #error "CascadeShadowSampling.hlsli needs CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER"
#endif

#ifndef CASCADE_SHADOW_SAMPLER_POINT_REGISTER
    #error "CascadeShadowSampling.hlsli needs CASCADE_SHADOW_SAMPLER_POINT_REGISTER"
#endif

// The SHADOW_FILTER_FUNCTION_* and SHADOW_FILTER_MODE_* values come from CascadeStructs.hlsli.
#if !defined(SHADOW_FILTER_FUNCTION)
    #define SHADOW_FILTER_FUNCTION SHADOW_FILTER_FUNCTION_GRID
#endif

#define FILTER_FUNCTION_GRID         (SHADOW_FILTER_FUNCTION == SHADOW_FILTER_FUNCTION_GRID)
#define FILTER_FUNCTION_POISSON_DISK (SHADOW_FILTER_FUNCTION == SHADOW_FILTER_FUNCTION_POISSON_DISK)
#define FILTER_FUNCTION_VOGEL_DISK   (SHADOW_FILTER_FUNCTION == SHADOW_FILTER_FUNCTION_VOGEL_DISK)

#if !defined(SHADOW_FILTER_MODE)
    #define SHADOW_FILTER_MODE SHADOW_FILTER_MODE_PCF
#endif

#define FILTER_MODE_PCF  1
#define FILTER_MODE_PCSS 0

#if !defined(NUM_SAMPLES)
    #define NUM_SAMPLES 32
#endif

#if !defined(ROTATE_SAMPLES)
    #define ROTATE_SAMPLES 1
#endif

#if !defined(NUM_BLOCKER_SAMPLES)
    #define NUM_BLOCKER_SAMPLES 128
#endif

#if !defined(SELECT_CASCADE_FROM_PROJECTION)
    #define SELECT_CASCADE_FROM_PROJECTION 1
#endif

#if !defined(ENABLE_CASCADE_BLENDING)
    #define ENABLE_CASCADE_BLENDING 1
#endif

#if !defined(CASCADE_FADE_FACTOR)
    #define CASCADE_FADE_FACTOR 0.05
#endif

// Debugging defines
#if !defined(ENABLE_FIRST_CASCADE_ONLY)
    #define ENABLE_FIRST_CASCADE_ONLY 0
#endif

#if !defined(MAX_PCSS_FILTER_SIZE)
    #define MAX_PCSS_FILTER_SIZE 0.999
#endif
#if !defined(MIN_PCSS_FILTER_SIZE)
    #define MIN_PCSS_FILTER_SIZE 0.03
#endif
#if !defined(PENUMBRA_SCALE)
    #define PENUMBRA_SCALE 30.0
#endif

#if !defined(USE_ORTHO)
    #define USE_ORTHO 1
#endif

StructuredBuffer<FCascadeSplit> ShadowSplitsBuffer : register(CASCADE_SHADOW_SPLITS_REGISTER);
Texture2DArray<float>           ShadowCascades     : register(CASCADE_SHADOW_CASCADES_REGISTER);

SamplerComparisonState ShadowSamplerPointCmp  : register(CASCADE_SHADOW_SAMPLER_POINT_CMP_REGISTER);
SamplerComparisonState ShadowSamplerLinearCmp : register(CASCADE_SHADOW_SAMPLER_LINEAR_CMP_REGISTER);
SamplerState           ShadowSamplerPoint     : register(CASCADE_SHADOW_SAMPLER_POINT_REGISTER);

// The runtime half of the tuning. Everything else is a compile-time knob above, because it changes
// the shape of the filter loops rather than their extent.
struct FCascadeShadowSettings
{
    float FilterSize;
    float MaxFilterSize;
    uint  ShadowMapSize;
    uint  NumSamples;
};

struct FCascadeShadowContext
{
    FDirectionalLight      Light;
    FCascadeShadowSettings Settings;
};

struct FFilterSetup
{
    float3   PositionWS;
    float3   Normal;
    float2   ShadowPosition;
    float    BiasedDepth;
#if ROTATE_SAMPLES
    float2x2 SampleRotationMatrix;
#endif
};

float2 GenerateSampleOffset(FCascadeShadowContext Context, uint SampleIndex)
{
#if FILTER_FUNCTION_POISSON_DISK
        // Sample from predefined poisson-disks
    #if (NUM_SAMPLES == 16)
        return PoissonDisk16[SampleIndex];
    #elif (NUM_SAMPLES == 32)
        return PoissonDisk32[SampleIndex];
    #elif (NUM_SAMPLES == 64)
        return PoissonDisk64[SampleIndex];
    #elif (NUM_SAMPLES == 128)
        return PoissonDisk128[SampleIndex];
    #else
        return 0.0;
    #endif
#else
    // Generate a vogel sample using runtime sample count
    return VogelDiskSample(SampleIndex, Context.Settings.NumSamples, 1);
#endif
}

float2 GenerateBlockerSampleOffset(uint SampleIndex)
{
#if FILTER_FUNCTION_POISSON_DISK
        // Sample from predefined poisson-disks
    #if (NUM_BLOCKER_SAMPLES == 16)
        return PoissonDisk16[SampleIndex];
    #elif (NUM_BLOCKER_SAMPLES == 32)
        return PoissonDisk32[SampleIndex];
    #elif (NUM_BLOCKER_SAMPLES == 64)
        return PoissonDisk64[SampleIndex];
    #elif (NUM_BLOCKER_SAMPLES == 128)
        return PoissonDisk128[SampleIndex];
    #else
        return 0.0;
    #endif
#else
    // Generate a vogel sample
    return VogelDiskSample(SampleIndex, NUM_BLOCKER_SAMPLES, 1);
#endif
}

float2 ComputeBlockerDepth(FCascadeShadowContext Context, uint CascadeIndex, FFilterSetup FilterSetup, float SearchSize)
{
    const float FilterSize = 0.1;

    // Calculate the size of the filter
    float2 FilterRadius = SearchSize * FilterSize.xx * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy);

    // Find blockers
    float NumBlockers     = 0.0;
    float BlockerDepthSum = 0.0;

    for (int Sample = 0; Sample < NUM_BLOCKER_SAMPLES; ++Sample)
    {
        float2 SampleOffset = GenerateBlockerSampleOffset(Sample);
    #if ROTATE_SAMPLES
        SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
    #endif
        SampleOffset = SampleOffset * FilterRadius;

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

float ShadowAmountPCSS(FCascadeShadowContext Context, uint CascadeIndex, FFilterSetup FilterSetup, float PenumbraSize)
{
    // Calculate the size of the filter
    float2 FilterSize = PenumbraSize.xx * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy);

    float Result = 0.0;

    // [branch]
    // if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float2 FilterRadius = FilterSize;

    #if FILTER_FUNCTION_VOGEL_DISK
        const uint EffectiveNumSamples = Context.Settings.NumSamples;
    #else
        const uint EffectiveNumSamples = NUM_SAMPLES;
    #endif

        for (uint Sample = 0; Sample < EffectiveNumSamples; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Context, Sample);
        #if ROTATE_SAMPLES
            SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
        #endif
            SampleOffset = SampleOffset * FilterRadius;

            Result += ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinearCmp, float3(FilterSetup.ShadowPosition + SampleOffset, CascadeIndex), FilterSetup.BiasedDepth);
        }

        Result = Result / float(EffectiveNumSamples);
    }
    // else
    // {
    //     Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinearCmp, float3(FilterSetup.ShadowPosition, CascadeIndex), FilterSetup.BiasedDepth);
    // }

    return saturate(Result);
}

float ShadowAmountDiscPCF(FCascadeShadowContext Context, uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float2 MaxFilterSize = Context.Settings.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(Context.Settings.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, Context.Settings.MaxFilterSize);

    float Result = 0.0;

#if FILTER_FUNCTION_VOGEL_DISK
    const uint EffectiveNumSamples = Context.Settings.NumSamples;
#else
    const uint EffectiveNumSamples = NUM_SAMPLES;
#endif

    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float  ShadowMapSize = float(Context.Settings.ShadowMapSize);
        const float2 FilterRadius  = (FilterSize * 0.5) / ShadowMapSize;

        for (uint Sample = 0; Sample < EffectiveNumSamples; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Context, Sample);
        #if ROTATE_SAMPLES
            SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
        #endif
            SampleOffset = SampleOffset * FilterRadius;

            Result += ShadowCascades.SampleCmpLevelZero(ShadowSamplerPointCmp, float3(FilterSetup.ShadowPosition + SampleOffset, CascadeIndex), FilterSetup.BiasedDepth);
        }

        Result = Result / float(EffectiveNumSamples);
    }
    else
    {
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinearCmp, float3(FilterSetup.ShadowPosition, CascadeIndex), FilterSetup.BiasedDepth);
    }

    return saturate(Result);
}

float ShadowAmountGridPCF(FCascadeShadowContext Context, uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float2 MaxFilterSize = Context.Settings.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(Context.Settings.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, Context.Settings.MaxFilterSize);

    float Result = 0.0;

    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float  ShadowMapSize = float(Context.Settings.ShadowMapSize);
        const float  TexelSize     = 1.0 / ShadowMapSize;
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

float PCSS_SearchRadiusUV(float DepthVS)
{
    return saturate(DepthVS - 0.1) / DepthVS;
}

float PCSS_PenumbraRadius(float RecieverDepthVS, float BlockerDepthVS, float LightSize)
{
    // return LightSize * abs((RecieverDepthVS - BlockerDepthVS) / (BlockerDepthVS + FLT_MIN));

    return /*LightSize * */ abs(RecieverDepthVS - BlockerDepthVS);// / (BlockerDepthVS + FLT_MIN);
}

float PCSS_PenumbraRadiusUV(float RecieverDepthVS, float BlockerDepthVS)
{
    return abs(RecieverDepthVS - BlockerDepthVS) / BlockerDepthVS;
}

float PCSS_ProjectToLightUV(FCascadeShadowContext Context, float PenumbraRadiusUV, float DepthVS, float NearPlane)
{
	return Context.Light.LightSize * PenumbraRadiusUV * NearPlane / DepthVS;
}

float PCSS_ClipToEye(float DepthVS, float NearPlane, float FarPlane)
{
#if USE_ORTHO
    return NearPlane + (FarPlane - NearPlane) * DepthVS;
#else
	return FarPlane * NearPlane / (FarPlane - DepthVS * (FarPlane - NearPlane));
#endif
}

float CascadeShadowAmount(FCascadeShadowContext Context, uint CascadeIndex, float3 PositionWS, float3 NormalWS, float3 ShadowPosition, inout uint RandomSeed)
{
    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];
    ShadowPosition += CascadeSplit.Offsets.xyz;
    ShadowPosition *= CascadeSplit.Scale.xyz;

    // Calculate Biased Depth
    const float BiasScale   = 1.0 - saturate(dot(NormalWS, normalize(Context.Light.Direction)));
    const float ShadowBias  = max(max(0.0001, Context.Light.ShadowBias) * BiasScale, 0.0005);
    const float BiasedDepth = saturate(ShadowPosition.z - ShadowBias);

    FFilterSetup FilterSetup;
    FilterSetup.PositionWS     = PositionWS;
    FilterSetup.Normal         = NormalWS;
    FilterSetup.ShadowPosition = ShadowPosition.xy;
    FilterSetup.BiasedDepth    = BiasedDepth;

#if ROTATE_SAMPLES
    float Theta    = NextRandom(RandomSeed) * PI_2;
    float CosTheta = cos(Theta);
    float SinTheta = sin(Theta);
    FilterSetup.SampleRotationMatrix = float2x2(float2(CosTheta, -SinTheta), float2(SinTheta,  CosTheta));
#endif

#if FILTER_MODE_PCSS
    // PCSS Step 1: With PCSS we need to calculate the number of blockers and the average depth
    // FCascadeMatrices CascadeMatrices = ShadowMatricesBuffer[CascadeIndex];

    // float4 PositionVS = mul(float4(PositionWS, 1.0), UnpackMatrix(CascadeMatrices.View));
    // PositionVS.xyz /= PositionVS.w;

    // Calculate the blocker search radius
    const float BlockerSearchSizeUV = PCSS_SearchRadiusUV(ShadowPosition.z);

    // In case we did not find any blockers, then we can just stop here
    const float2 BlockerInfo = ComputeBlockerDepth(Context, CascadeIndex, FilterSetup, BlockerSearchSizeUV);
    if (BlockerInfo.y < 1.0)
    {
        return 1.0;
    }

    return BlockerInfo.y;

    // PCSS Step 2: Penumbra size
    const float AvgBlockerDepth = BlockerInfo.x;
    // const float AvgBlockerDepthVS = PCSS_ClipToEye(AvgBlockerDepth, CascadeSplit.NearPlane, CascadeSplit.FarPlane);
    // const float PenumbraWidth     = PCSS_PenumbraRadiusUV(PositionVS.z, AvgBlockerDepthVS);
    // const float PenumbraRadius    = PCSS_ProjectToLightUV(Context, PenumbraWidth, PositionVS.z, CascadeSplit.NearPlane);
    float PenumbraRadius = PCSS_PenumbraRadius(BiasedDepth, AvgBlockerDepth, Context.Light.LightSize);

    // return PenumbraRadius * abs(ShadowSplitsBuffer[CascadeIndex].Scale.x);

    // PCSS Step 3: Filter the shadows
    return ShadowAmountPCSS(Context, CascadeIndex, FilterSetup, PenumbraRadius);
#elif FILTER_MODE_PCF
    #if FILTER_FUNCTION_GRID
        // PCF using a grid
        return ShadowAmountGridPCF(Context, CascadeIndex, FilterSetup);
    #else
        // PCF using a random disk (Poisson or Vogel)
        return ShadowAmountDiscPCF(Context, CascadeIndex, FilterSetup);
    #endif
#else
    // Fallback to a single sample
    return ShadowAmountSimple(CascadeIndex, FilterSetup);
#endif
}

// ViewPosZ is the receiver's view-space depth, which the caller already has one way or another:
// the shadow mask converts it from the depth buffer, a forward pass from its own SV_Position.
float ComputeCascadeShadow(FCascadeShadowContext Context, float3 PositionWS, float3 Normal, float ViewPosZ, inout uint CascadeIndex, inout uint RandomSeed)
{
    const float3 ProjectionPosition = mul(float4(PositionWS, 1.0), Context.Light.ShadowMatrix).xyz;

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
        if (ViewPosZ < CascadeSplit.Split)
        {
            CascadeIndex = Index;
        }
    #endif
    }

#if ENABLE_FIRST_CASCADE_ONLY
    if (CascadeIndex > 0)
    {
        return 1.0;
    }
#endif

    // Calculate shadow factor
    float ShadowAmount = CascadeShadowAmount(Context, CascadeIndex, PositionWS, Normal, ProjectionPosition, RandomSeed);

// Blend between this and next cascade
#if ENABLE_CASCADE_BLENDING && !ENABLE_FIRST_CASCADE_ONLY
    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];

    float NextSplit  = CascadeSplit.Split;
    float SplitSize  = NextSplit - CascadeSplit.PreviousSplit;
    float FadeFactor = (NextSplit - ViewPosZ) / SplitSize;

    #if SELECT_CASCADE_FROM_PROJECTION
        const float4 Offsets = CascadeSplit.Offsets;
        const float4 Scale   = CascadeSplit.Scale;

        float3 CascadePosition  = ProjectionPosition + Offsets.xyz;
        CascadePosition *= Scale.xyz;
        CascadePosition  = abs(CascadePosition * 2.0 - 1.0);

        float DistToEdge = 1.0 - max(max(CascadePosition.x, CascadePosition.y), CascadePosition.z);
        FadeFactor = max(DistToEdge, FadeFactor);
    #endif

    [branch]
    if(FadeFactor <= CASCADE_FADE_FACTOR && CascadeIndex != (NUM_SHADOW_CASCADES - 1))
    {
        const float NextSplitVisibility = CascadeShadowAmount(Context, CascadeIndex + 1, PositionWS, Normal, ProjectionPosition, RandomSeed);
        const float LerpAmount = smoothstep(0.0, CASCADE_FADE_FACTOR, FadeFactor);
        ShadowAmount = lerp(NextSplitVisibility, ShadowAmount, LerpAmount);
    }
#endif

    return ShadowAmount;
}

#endif // CASCADE_SHADOW_SAMPLING_HLSLI
