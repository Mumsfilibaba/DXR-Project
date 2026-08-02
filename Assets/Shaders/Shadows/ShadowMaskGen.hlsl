#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "PoissonDisk.hlsli"
#include "VogelDisk.hlsli"
#include "Halton.hlsli"
#include "CascadeStructs.hlsli"

#if !defined(NUM_THREADS)
    #define NUM_THREADS 16
#endif

#if !defined(ENABLE_DEBUG)
    #define ENABLE_DEBUG 0
#endif

#if !defined(FILTER_FUNCTION_GRID)
    #define FILTER_FUNCTION_GRID 0
#endif

#if !defined(FILTER_FUNCTION_POISSON_DISK)
    #define FILTER_FUNCTION_POISSON_DISK 0
#endif

#if !defined(FILTER_FUNCTION_VOGEL_DISK)
    #define FILTER_FUNCTION_VOGEL_DISK 0
#endif

#if !defined(FILTER_MODE_PCF)
    #define FILTER_MODE_PCF 1
#endif

#if !defined(FILTER_MODE_PCSS)
    #define FILTER_MODE_PCSS 0
#endif

// PCSS is disabled for now
#if defined(FILTER_MODE_PCSS) 
    #undef FILTER_MODE_PCSS
    #undef FILTER_MODE_PCF
    #define FILTER_MODE_PCSS 0
    #define FILTER_MODE_PCF 1
#endif

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

struct FDirectionalShadowSettings
{
    // 0-16
    float FilterSize;
    float MaxFilterSize;
    uint  ShadowMapSize;
    uint  FrameIndex;

    // 16-32
    uint  NumSamples;
    uint  Padding0;
    uint  Padding1;
    uint  Padding2;
};

#if SHADER_BACKEND == SHADER_BACKEND_METAL
    ConstantBuffer<FCamera>           CameraBuffer : register(b2);
    ConstantBuffer<FDirectionalLight> LightBuffer  : register(b3);
#else
    ConstantBuffer<FCamera>           CameraBuffer : register(b0);
    ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);
#endif

ConstantBuffer<FDirectionalShadowSettings> SettingsBuffer : register(b2);

StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<FCascadeSplit>    ShadowSplitsBuffer   : register(t1);
Texture2D<float>                   DepthBuffer          : register(t2);
Texture2D<float3>                  NormalBuffer         : register(t3);
Texture2DArray<float>              ShadowCascades       : register(t4);

// Output
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> Output : register(u0);
#if ENABLE_DEBUG
    TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint> CascadeIndexTex : register(u1);
#endif

// Samplers
SamplerComparisonState ShadowSamplerPointCmp  : register(s0);
SamplerComparisonState ShadowSamplerLinearCmp : register(s1);
SamplerState           ShadowSamplerPoint     : register(s2);

float2 GenerateSampleOffset(uint SampleIndex)
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
    return VogelDiskSample(SampleIndex, SettingsBuffer.NumSamples, 1);
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

float GetShadowMapSize()
{
    return (float)SettingsBuffer.ShadowMapSize;
}

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

float2 ComputeBlockerDepth(uint CascadeIndex, FFilterSetup FilterSetup, float SearchSize)
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

float ShadowAmountPCSS(uint CascadeIndex, FFilterSetup FilterSetup, float PenumbraSize)
{
    // Calculate the size of the filter
    float2 FilterSize = PenumbraSize.xx * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy);

    float Result = 0.0;

    // [branch]
    // if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float2 FilterRadius = FilterSize;

    #if FILTER_FUNCTION_VOGEL_DISK
        const uint EffectiveNumSamples = SettingsBuffer.NumSamples;
    #else
        const uint EffectiveNumSamples = NUM_SAMPLES;
    #endif

        for (uint Sample = 0; Sample < EffectiveNumSamples; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Sample);
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

float ShadowAmountDiscPCF(uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
#if FILTER_FUNCTION_VOGEL_DISK
    const uint EffectiveNumSamples = SettingsBuffer.NumSamples;
#else
    const uint EffectiveNumSamples = NUM_SAMPLES;
#endif

    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float  ShadowMapSize = GetShadowMapSize();
        const float2 FilterRadius  = (FilterSize * 0.5) / ShadowMapSize;

        for (uint Sample = 0; Sample < EffectiveNumSamples; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Sample);
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

float ShadowAmountGridPCF(uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float  ShadowMapSize = GetShadowMapSize();
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

float PCSS_ProjectToLightUV(float PenumbraRadiusUV, float DepthVS, float NearPlane)
{
	return LightBuffer.LightSize * PenumbraRadiusUV * NearPlane / DepthVS;
}

float PCSS_ClipToEye(float DepthVS, float NearPlane, float FarPlane)
{
#if USE_ORTHO
    return NearPlane + (FarPlane - NearPlane) * DepthVS;
#else
	return FarPlane * NearPlane / (FarPlane - DepthVS * (FarPlane - NearPlane));
#endif
}

float CascadeShadowAmount(uint CascadeIndex, float3 PositionWS, float3 NormalWS, float3 ShadowPosition, inout uint RandomSeed)
{
    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];
    ShadowPosition += CascadeSplit.Offsets.xyz;
    ShadowPosition *= CascadeSplit.Scale.xyz;

    // Calculate Biased Depth
    const float BiasScale   = 1.0 - saturate(dot(NormalWS, normalize(LightBuffer.Direction)));
    const float ShadowBias  = max(max(0.0001, LightBuffer.ShadowBias) * BiasScale, 0.0005);
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
    const float2 BlockerInfo = ComputeBlockerDepth(CascadeIndex, FilterSetup, BlockerSearchSizeUV);
    if (BlockerInfo.y < 1.0)
    {
        return 1.0;
    }

    return BlockerInfo.y;

    // PCSS Step 2: Penumbra size
    const float AvgBlockerDepth = BlockerInfo.x;
    // const float AvgBlockerDepthVS = PCSS_ClipToEye(AvgBlockerDepth, CascadeSplit.NearPlane, CascadeSplit.FarPlane);
    // const float PenumbraWidth     = PCSS_PenumbraRadiusUV(PositionVS.z, AvgBlockerDepthVS);
    // const float PenumbraRadius    = PCSS_ProjectToLightUV(PenumbraWidth, PositionVS.z, CascadeSplit.NearPlane);
    float PenumbraRadius = PCSS_PenumbraRadius(BiasedDepth, AvgBlockerDepth, LightBuffer.LightSize);

    // return PenumbraRadius * abs(ShadowSplitsBuffer[CascadeIndex].Scale.x);

    // PCSS Step 3: Filter the shadows
    return ShadowAmountPCSS(CascadeIndex, FilterSetup, PenumbraRadius);
#elif FILTER_MODE_PCF
    #if FILTER_FUNCTION_GRID
        // PCF using a grid
        return ShadowAmountGridPCF(CascadeIndex, FilterSetup);
    #else
        // PCF using a random disk (Poisson or Vogel)
        return ShadowAmountDiscPCF(CascadeIndex, FilterSetup);
    #endif
#else 
    // Fallback to a single sample 
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
    float ShadowAmount = CascadeShadowAmount(CascadeIndex, PositionWS, Normal, ProjectionPosition, RandomSeed);

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
        const float NextSplitVisibility = CascadeShadowAmount(CascadeIndex + 1, PositionWS, Normal, ProjectionPosition, RandomSeed);
        const float LerpAmount = smoothstep(0.0, CASCADE_FADE_FACTOR, FadeFactor);
        ShadowAmount = lerp(NextSplitVisibility, ShadowAmount, LerpAmount);
    }
#endif

    return ShadowAmount;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(uint3 DispatchThreadID : SV_DispatchThreadID)
{
    const uint2 Pixel = DispatchThreadID.xy;
   
    // Discard pixels not rendered to the GBuffer
    const float Depth = DepthBuffer.Load(int3(Pixel, 0)); 
    if (Depth == 1.0)
    {
        Output[Pixel] = 1.0;
        return;
    }

    const float2 PixelCenter   = float2(Pixel) + 0.5;
    const float2 TexCoord      = PixelCenter / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float3 PositionWS    = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0));
    const float3 Normal        = UnpackNormal(GBufferNormal);

    // Initialize random-seed
    uint RandomSeed = InitRandom(Pixel, CameraBuffer.ViewportWidth, 0);
    
    // Cascade-index is written to for debug purposes
    uint CascadeIndex = 0;

    // Calculate the Shadow
    const float ShadowAmount = ComputeShadow(PositionWS, Normal, Depth, CascadeIndex, RandomSeed);
    Output[Pixel] = ShadowAmount;

    // Output debug-information needed when visualizing the cascades
#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}