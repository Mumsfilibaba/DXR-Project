#include "../Structs.hlsli"
#include "../Random.hlsli"
#include "../Helpers.hlsli"
#include "../PoissonDisk.hlsli"
#include "../VogelDisk.hlsli"
#include "../Halton.hlsli"
#include "CascadeStructs.hlsli"

#if !defined(NUM_THREADS)
    #define NUM_THREADS 16
#endif

#if !defined(ENABLE_DEBUG)
    #define ENABLE_DEBUG 0
#endif

#if !defined(FILTER_FUNCTION_POISSON_DISK)
    #define FILTER_FUNCTION_POISSON_DISK 0
#endif

#if !defined(FILTER_FUNCTION_VOGEL_DISK)
    #define FILTER_FUNCTION_VOGEL_DISK 0
#endif

#if !defined(FILTER_FUNCTION_INTERLEAVED_GRADIENT_NOISE)
    #define FILTER_FUNCTION_INTERLEAVED_GRADIENT_NOISE 0
#endif

#if !defined(FILTER_MODE_PCF)
    #define FILTER_MODE_PCF 1
#endif

#if !defined(FILTER_MODE_PCSS)
    #define FILTER_MODE_PCSS 0
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

#if !defined(ENABLE_CASCADE_FALLBACK)
    #define ENABLE_CASCADE_FALLBACK 0
#endif

#if !defined(CASCADE_FADE_FACTOR)
    #define CASCADE_FADE_FACTOR 0.10
#endif

// Debugging defines
#define ENABLE_FIRST_CASCADE_ONLY 0

#define SHADOW_DEBUG_NONE               0
#define SHADOW_DEBUG_CASCADE_INDEX      1
#define SHADOW_DEBUG_CASCADE_TRANSITION 2
#define SHADOW_DEBUG_FILTER_MARGIN      3
#define SHADOW_DEBUG_PCSS_RADIUS_CLAMP  4
#define SHADOW_DEBUG_CASCADE_UPDATED    5
#define SHADOW_DEBUG_CONTAINMENT        6
#define SHADOW_DEBUG_CASCADE_FALLBACK   7

// NOTE: DirectionalLight.LightSize is treated as *degrees* (angular diameter).

#define USE_ORTHO 1

// Camera and Light
#if SHADER_LANG == SHADER_LANG_MSL
ConstantBuffer<FCamera> CameraBuffer : register(b2);
ConstantBuffer<FDirectionalLight> LightBuffer : register(b3);
#else
ConstantBuffer<FCamera> CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer : register(b1);
#endif

struct FDirectionalShadowSettings
{
    float PCFFilterWorld;
    float PCFMinFilterRadiusTexels;
    uint  ShadowMapSize;
    uint  FrameIndex;

    // PCSS tuning
    float PCSSRadiusScale;
    float PCSSBlockerSearchScale;
    float PCSSMinFilterRadiusTexels;
    float PCSSPadding0;

    float PCSSBlockerSamplingClump;
    float PCSSMaxPenumbraWorld;
    float PCSSMaxSearchDistanceWorld;
    float PCSSMinFilterMaxAngularDiameter;
    float PCSSBlockerSearchAngularDiameter;
    float ShadowMaxDistance;
    float ShadowMaxDistanceFade;
    float Padding3;

    uint  ShadowDebugMode;
    uint  ShadowDebugPadding0;
    uint  ShadowDebugPadding1;
    uint  ShadowDebugPadding2;
};

ConstantBuffer<FDirectionalShadowSettings> SettingsBuffer : register(b2);

// Shadow information
StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<FCascadeSplit> ShadowSplitsBuffer : register(t1);

// G-Buffer
Texture2D<float> DepthBuffer : register(t2);
Texture2D<float3> NormalBuffer : register(t3);

// Shadow Cascades
Texture2DArray<float> ShadowCascades : register(t4);

// Output
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float> Output : register(u0);
#if ENABLE_DEBUG
TEXTURE_FORMAT_UNKNOWN RWTexture2D<uint> CascadeIndexTex : register(u1);
#endif
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> ShadowDebugBuffer : register(u2);

// Samplers
SamplerComparisonState ShadowSamplerPointCmp : register(s0);
SamplerComparisonState ShadowSamplerLinearCmp : register(s1);

SamplerState ShadowSamplerPoint : register(s2);

float InterleavedGradientNoise(float2 Pixel, uint FrameIndex)
{
    // https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
    const float3 Magic = float3(0.06711056, 0.00583715, 52.9829189);
    return frac(Magic.z * frac(dot(Pixel, Magic.xy) + float(FrameIndex) * 0.0001));
}

float2 ConcentricSampleDisk(float2 U)
{
    const float2 UOffset = U * 2.0 - 1.0;
    if (UOffset.x == 0.0 && UOffset.y == 0.0)
    {
        return float2(0.0, 0.0);
    }

    float R;
    float Theta;

    if (abs(UOffset.x) > abs(UOffset.y))
    {
        R     = UOffset.x;
        Theta = (PI * 0.25) * (UOffset.y / UOffset.x);
    }
    else
    {
        R     = UOffset.y;
        Theta = (PI * 0.5) - (PI * 0.25) * (UOffset.x / UOffset.y);
    }

    return R * float2(cos(Theta), sin(Theta));
}

uint StableShadowSeed(float3 PositionWS)
{
    // Use cascade-0 reference for stable, cascade-independent sampling.
    const float RefWorldTexelSize = max(ShadowSplitsBuffer[0].RefWorldTexelSize, 1e-6);
    const float QuantStep = clamp(RefWorldTexelSize * 0.25, 0.005, 0.1);
    int3 Q = int3(floor(PositionWS / QuantStep));
    return Hash3(asuint(Q));
}

float2 GenerateIGNDiskSample(uint SampleIndex, uint SampleCount, uint StableSeed)
{
    // R2 sequence + Cranley-Patterson rotation from interleaved gradient noise.
    // Produces a stable per-pixel, low-discrepancy distribution without lookup tables.
    const float2 R2 = float2(0.754877666, 0.569840296);

    float2 U = frac(float2(0.5, 0.5) + float(SampleIndex + 1) * R2);
    const float2 CP = HashToFloat2(HashCombine(StableSeed, 0x68bc21ebu));

    U = frac(U + CP);
    return ConcentricSampleDisk(U);
}

float2 GenerateSampleOffset(uint SampleIndex, uint StableSeed)
{
#if FILTER_FUNCTION_INTERLEAVED_GRADIENT_NOISE
    return GenerateIGNDiskSample(SampleIndex, NUM_SAMPLES, StableSeed);

#elif FILTER_FUNCTION_POISSON_DISK

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

#elif FILTER_FUNCTION_VOGEL_DISK
    // Generate a vogel sample
    return VogelDiskSample(SampleIndex, NUM_SAMPLES, 1);

#else
    // Fallback to vogel disk
    return VogelDiskSample(SampleIndex, NUM_SAMPLES, 1);
#endif
}

float2 GenerateBlockerSampleOffset(uint SampleIndex, uint StableSeed)
{
#if FILTER_FUNCTION_INTERLEAVED_GRADIENT_NOISE
    return GenerateIGNDiskSample(SampleIndex, NUM_BLOCKER_SAMPLES, StableSeed);

#elif FILTER_FUNCTION_POISSON_DISK

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

#elif FILTER_FUNCTION_VOGEL_DISK
    // Generate a vogel sample
    return VogelDiskSample(SampleIndex, NUM_BLOCKER_SAMPLES, 1);

#else
    return VogelDiskSample(SampleIndex, NUM_BLOCKER_SAMPLES, 1);
#endif
}

struct FFilterSetup
{
    float3 PositionWS;
    float3 Normal; 
    float2 ShadowPosition;
    float  BiasedDepth;
    float  ReceiverDepth;
    uint2  Pixel;
    float3 GlobalShadowPosition;
    float  ShadowBiasWorld;
    uint   StableSeed;
    uint   Padding0;

#if ROTATE_SAMPLES
    float2x2 SampleRotationMatrix;
#endif
};

#define CASCADE_UV_EPSILON (1e-4)

bool IsInsideCascadeUV(float2 UV)
{
    const float2 MinUV = float2(-CASCADE_UV_EPSILON, -CASCADE_UV_EPSILON);
    const float2 MaxUV = float2(1.0 + CASCADE_UV_EPSILON, 1.0 + CASCADE_UV_EPSILON);
    return all(UV >= MinUV) && all(UV <= MaxUV);
}

bool IsInsideCascadeUVW(float3 UVW)
{
    const float3 MinUVW = float3(-CASCADE_UV_EPSILON, -CASCADE_UV_EPSILON, -CASCADE_UV_EPSILON);
    const float3 MaxUVW = float3(1.0 + CASCADE_UV_EPSILON, 1.0 + CASCADE_UV_EPSILON, 1.0 + CASCADE_UV_EPSILON);
    return all(UVW >= MinUVW) && all(UVW <= MaxUVW);
}

float3 GlobalToCascadeUVW(uint CascadeIndex, float3 GlobalShadowPos)
{
    FCascadeSplit Split = ShadowSplitsBuffer[CascadeIndex];
    return (GlobalShadowPos + Split.Offsets.xyz) * Split.Scale.xyz;
}

float3 ComputeCascadeUVWFromMatrix(uint CascadeIndex, float3 PositionWS)
{
    const float4 Clip = mul(float4(PositionWS, 1.0), ShadowMatricesBuffer[CascadeIndex].ViewProj);
    const float InvW = (abs(Clip.w) > 1e-6) ? rcp(Clip.w) : 0.0;
    const float3 NDC = Clip.xyz * InvW;

    // Match the texture-space convention used in CascadeMatrixGen.hlsl (Y flipped).
    const float2 UV = NDC.xy * float2(0.5, -0.5) + 0.5;
    return float3(UV, NDC.z);
}

float ShadowCompare(uint CascadeIndex, float ReceiverDepth, float SampleDepth)
{
    return (ReceiverDepth <= SampleDepth) ? 1.0 : 0.0;
}

bool ShadowIsBlocker(uint CascadeIndex, float ReceiverDepth, float SampleDepth)
{
    return (SampleDepth < ReceiverDepth);
}

float CascadeDepthToGlobalDepth(uint CascadeIndex, float CascadeDepth)
{
    FCascadeSplit Split = ShadowSplitsBuffer[CascadeIndex];
    // Global space is the output of LightBuffer.ShadowMatrix (NDC for XY, light-view depth for Z).
    // Convert cascade UVW.z back to global light-view depth.
    const float ScaleZ = Split.Scale.z;
    const float InvScaleZ = (abs(ScaleZ) > 1e-6) ? rcp(ScaleZ) : 0.0;
    return CascadeDepth * InvScaleZ - Split.Offsets.z;
}

float ReceiverDepthForCascade(uint CascadeIndex, float3 ReceiverGlobalShadowPos)
{
    FCascadeSplit Split = ShadowSplitsBuffer[CascadeIndex];
    const float3 ReceiverUVW = (ReceiverGlobalShadowPos + Split.Offsets.xyz) * Split.Scale.xyz;
    return saturate(ReceiverUVW.z);
}

float ReceiverBiasedDepthForCascade(uint CascadeIndex, float3 ReceiverGlobalShadowPos, float ShadowBiasWorld)
{
    FCascadeSplit Split = ShadowSplitsBuffer[CascadeIndex];
    const float3 ReceiverUVW = (ReceiverGlobalShadowPos + Split.Offsets.xyz) * Split.Scale.xyz;
    const float BiasDepth = ShadowBiasWorld * abs(Split.Scale.z);
    return saturate(ReceiverUVW.z - BiasDepth);
}

// Converts a base-cascade UV offset to a global shadow-space offset (XY only).
float2 BaseCascadeOffsetUVToGlobal(uint BaseCascadeIndex, float2 OffsetUV)
{
    const float2 BaseScale = max(ShadowSplitsBuffer[BaseCascadeIndex].Scale.xy, float2(1e-6, 1e-6));
    return OffsetUV / BaseScale;
}

// Samples the shadow-map depth for a given UV offset, automatically falling back to coarser cascades if the sample goes out of bounds.
float SampleShadowDepthMultiCascade(uint BaseCascadeIndex, FFilterSetup FilterSetup, float2 OffsetUV, out float OutReceiverBiasedDepth, out uint OutSampleCascadeIndex, inout uint FallbackHit)
{
    const float2 BaseUV = FilterSetup.ShadowPosition + OffsetUV;
    if (IsInsideCascadeUV(BaseUV))
    {
        OutReceiverBiasedDepth = FilterSetup.ReceiverDepth;
        OutSampleCascadeIndex = BaseCascadeIndex;
        return ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(BaseUV, BaseCascadeIndex), 0);
    }

#if ENABLE_CASCADE_FALLBACK
    const float2 GlobalOffset = BaseCascadeOffsetUVToGlobal(BaseCascadeIndex, OffsetUV);
    const float3 SampleGlobal = FilterSetup.GlobalShadowPosition + float3(GlobalOffset, 0.0);

    [loop]
    for (uint CascadeIndex = BaseCascadeIndex + 1; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        const float3 SampleUVW = GlobalToCascadeUVW(CascadeIndex, SampleGlobal);
        if (IsInsideCascadeUVW(SampleUVW))
        {
            OutReceiverBiasedDepth = ReceiverDepthForCascade(CascadeIndex, FilterSetup.GlobalShadowPosition);
            OutSampleCascadeIndex = CascadeIndex;
            FallbackHit = 1;
            return ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(SampleUVW.xy, CascadeIndex), 0);
        }
    }
#endif

    // Outside all cascades: treat as far depth (no blocker).
    OutReceiverBiasedDepth = FilterSetup.ReceiverDepth;
    OutSampleCascadeIndex = BaseCascadeIndex;
    return 1.0;
}

float SampleShadowCmpMultiCascade(uint BaseCascadeIndex, FFilterSetup FilterSetup, float2 OffsetUV, inout uint FallbackHit)
{
    const float2 BaseUV = FilterSetup.ShadowPosition + OffsetUV;
    if (IsInsideCascadeUV(BaseUV))
    {
        const float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(BaseUV, BaseCascadeIndex), 0);
        return ShadowCompare(BaseCascadeIndex, FilterSetup.BiasedDepth, SampleDepth);
    }

#if ENABLE_CASCADE_FALLBACK
    const float2 GlobalOffset = BaseCascadeOffsetUVToGlobal(BaseCascadeIndex, OffsetUV);
    const float3 SampleGlobal = FilterSetup.GlobalShadowPosition + float3(GlobalOffset, 0.0);

    [loop]
    for (uint CascadeIndex = BaseCascadeIndex + 1; CascadeIndex < NUM_SHADOW_CASCADES; ++CascadeIndex)
    {
        const float3 SampleUVW = GlobalToCascadeUVW(CascadeIndex, SampleGlobal);
        if (IsInsideCascadeUVW(SampleUVW))
        {
            const float ReceiverBiasedDepth = ReceiverBiasedDepthForCascade(CascadeIndex, FilterSetup.GlobalShadowPosition, FilterSetup.ShadowBiasWorld);
            FallbackHit = 1;
            const float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(SampleUVW.xy, CascadeIndex), 0);
            return ShadowCompare(CascadeIndex, ReceiverBiasedDepth, SampleDepth);
        }
    }
#endif

    // Outside all cascades: treat as unshadowed.
    return 1.0;
}

float2 ComputeBlockerDepth(uint CascadeIndex, FFilterSetup FilterSetup, float2 SearchRadiusUV, inout uint FallbackHit)
{
    const float2 FilterRadiusUV = SearchRadiusUV;

    // Find blockers
    float NumBlockers     = 0.0;
    float BlockerDepthSum = 0.0;
    const float Clump    = max(SettingsBuffer.PCSSBlockerSamplingClump, 0.0);
    const float ClumpExp = max(1.0, Clump);

    for (int Sample = 0; Sample < NUM_BLOCKER_SAMPLES; ++Sample)
    {
        float2 SampleOffset = GenerateBlockerSampleOffset(Sample, FilterSetup.StableSeed);
    #if ROTATE_SAMPLES

        SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
    #endif

        // Optional center-weighting for blocker search (reduces noise at the cost of bias).
        if (Clump > 0.0)
        {
            const float Radius = length(SampleOffset);
            if (Radius > 0.0)
            {
                const float NewRadius = pow(Radius, ClumpExp);
                SampleOffset *= (NewRadius / Radius);
            }
        }

        SampleOffset = SampleOffset * FilterRadiusUV;

        // Accumulate blockers
        float ReceiverBiasedDepth = FilterSetup.ReceiverDepth;
        uint SampleCascadeIndex = CascadeIndex;
        const float SampleDepth = SampleShadowDepthMultiCascade(CascadeIndex, FilterSetup, SampleOffset, ReceiverBiasedDepth, SampleCascadeIndex, FallbackHit);
        if (ShadowIsBlocker(SampleCascadeIndex, ReceiverBiasedDepth, SampleDepth))
        {
            // Accumulate in global light-view depth so blocker averaging remains consistent
            // even if taps spill into other cascades.
            BlockerDepthSum += CascadeDepthToGlobalDepth(SampleCascadeIndex, SampleDepth);
            NumBlockers += 1.0;
        }
    }

    // Return the average blocker depth
    if (NumBlockers < 1.0)
    {
        return float2(0.0, 0.0);
    }

    return float2(BlockerDepthSum / NumBlockers, NumBlockers);
}

float ShadowAmountPCSS(uint CascadeIndex, FFilterSetup FilterSetup, float2 FilterRadiusUV, inout uint FallbackHit)
{
    float Result = 0.0;

    [branch]
    if (FilterRadiusUV.x > 0.0 || FilterRadiusUV.y > 0.0)
    {
        for (int Sample = 0; Sample < NUM_SAMPLES; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Sample, FilterSetup.StableSeed);
        #if ROTATE_SAMPLES
            SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
        #endif
            SampleOffset = SampleOffset * FilterRadiusUV;

            Result += SampleShadowCmpMultiCascade(CascadeIndex, FilterSetup, SampleOffset, FallbackHit);
        }

        Result = Result / float(NUM_SAMPLES);
    }
    else
    {
        const float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(FilterSetup.ShadowPosition, CascadeIndex), 0);
        Result = ShadowCompare(CascadeIndex, FilterSetup.BiasedDepth, SampleDepth);
    }

    return saturate(Result);
}

float ShadowAmountDiscPCF(uint CascadeIndex, FFilterSetup FilterSetup, inout uint FallbackHit)
{
    float Result = 0.0;
    
    const float PCFWorldSize = max(SettingsBuffer.PCFFilterWorld, 0.0);
    [branch]
    if (PCFWorldSize > 0.0)
    {
        const FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];
        const float2 CascadeExtentsXY = max((CascadeSplit.MaxExtent - CascadeSplit.MinExtent).xy, float2(0.0001, 0.0001));
        const float RefWorldTexelSize = max(ShadowSplitsBuffer[0].RefWorldTexelSize, 1e-6);
        const float MinRadiusWorld = max(SettingsBuffer.PCFMinFilterRadiusTexels, 0.0) * RefWorldTexelSize;
        const float FilterWorld = max(PCFWorldSize, MinRadiusWorld);
        const float2 FilterRadius  = (FilterWorld * 0.5) / CascadeExtentsXY;

        for (int Sample = 0; Sample < NUM_SAMPLES; ++Sample)
        {
            float2 SampleOffset = GenerateSampleOffset(Sample, FilterSetup.StableSeed);
        #if ROTATE_SAMPLES
            SampleOffset = mul(SampleOffset, FilterSetup.SampleRotationMatrix);
        #endif
            SampleOffset = SampleOffset * FilterRadius;

            Result += SampleShadowCmpMultiCascade(CascadeIndex, FilterSetup, SampleOffset, FallbackHit);
        }

        Result = Result / float(NUM_SAMPLES);
    }
    else
    {
        const float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(FilterSetup.ShadowPosition, CascadeIndex), 0);
        Result = ShadowCompare(CascadeIndex, FilterSetup.BiasedDepth, SampleDepth);
    }

    return saturate(Result);
}

float ShadowAmountSimple(uint CascadeIndex, FFilterSetup FilterSetup)
{
    const float SampleDepth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(FilterSetup.ShadowPosition, CascadeIndex), 0);
    return ShadowCompare(CascadeIndex, FilterSetup.BiasedDepth, SampleDepth);
}

float PCSS_ClipToEye(float DepthVS, float NearPlane, float FarPlane)
{
#if USE_ORTHO
    return NearPlane + (FarPlane - NearPlane) * DepthVS;
#else
	return FarPlane * NearPlane / (FarPlane - DepthVS * (FarPlane - NearPlane));
#endif
}

float CascadeShadowAmount(uint CascadeIndex, float3 PositionWS, float3 NormalWS, float3 ShadowPosition, out float DebugPCSSClamp, out uint DebugFallbackHit)
{
    DebugPCSSClamp = 1.0;
    DebugFallbackHit = 0;

    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];

    // Bias terms
    const float3 LightDir   = normalize(LightBuffer.Direction);
    const float  BiasScale  = 1.0 - saturate(dot(NormalWS, LightDir));

    const float3 ReceiverPositionWS = PositionWS;
    float3 BiasedShadowPosition = ComputeCascadeUVWFromMatrix(CascadeIndex, PositionWS);

    // Outside cascade bounds => treat as unshadowed (avoids clamp/border artifacts).
    // This should be rare after the cascade containment fix in ComputeShadow.
    [branch]
    if (!IsInsideCascadeUVW(BiasedShadowPosition))
    {
        return 1.0;
    }

    // Calculate Biased Depth (receiver depth bias, in light clip-space)
    const float RefWorldTexelSize = max(CascadeSplit.RefWorldTexelSize, 1e-6);

    const float BaseBiasWorld = 0.5 * RefWorldTexelSize;
    float BiasDepth = BaseBiasWorld * abs(CascadeSplit.Scale.z);
    const float MinBiasDepth = max(1.0 / max(float(SettingsBuffer.ShadowMapSize), 1.0), 0.0005);
    BiasDepth = max(BiasDepth, MinBiasDepth);
    BiasDepth = max(BiasDepth, max(LightBuffer.ShadowBias, 0.0));
    BiasDepth *= (1.0 + BiasScale);

    const float RawDepth = saturate(BiasedShadowPosition.z);
    const float BiasedDepth = saturate(RawDepth - BiasDepth);

    FFilterSetup FilterSetup;
    FilterSetup.PositionWS     = ReceiverPositionWS;
    FilterSetup.Normal         = NormalWS;
    FilterSetup.ShadowPosition = BiasedShadowPosition.xy;
    FilterSetup.BiasedDepth    = BiasedDepth;
    FilterSetup.ReceiverDepth  = RawDepth;
    FilterSetup.GlobalShadowPosition = ShadowPosition;
    const float ShadowBiasWorld = BiasDepth / max(abs(CascadeSplit.Scale.z), 1e-6);
    FilterSetup.ShadowBiasWorld = ShadowBiasWorld;
    FilterSetup.Pixel          = 0;
    FilterSetup.StableSeed     = StableShadowSeed(PositionWS);
    FilterSetup.Padding0       = 0;
    
#if ROTATE_SAMPLES
    float Theta    = HashToFloat(HashCombine(FilterSetup.StableSeed, 0x9e3779b9u)) * PI_2;
    float CosTheta = cos(Theta);
    float SinTheta = sin(Theta);
    FilterSetup.SampleRotationMatrix = float2x2(float2(CosTheta, -SinTheta), float2(SinTheta,  CosTheta));
#endif

#if FILTER_MODE_PCSS
    
    // PCSS Step 0: Setup
    const float ReceiverDepthLS = PCSS_ClipToEye(saturate(BiasedShadowPosition.z), CascadeSplit.NearPlane, CascadeSplit.FarPlane);
    const float ReceiverDepthGlobalLS = FilterSetup.GlobalShadowPosition.z;
    const float2 CascadeExtentsXY = max((CascadeSplit.MaxExtent - CascadeSplit.MinExtent).xy, float2(0.0001, 0.0001));
    // Use cascade 0 reference to keep the PCSS clamp consistent across cascades.
    // This avoids visible seam changes in penumbra size when crossing cascade boundaries.
    const float RefWorldTexelSizeRef = max(ShadowSplitsBuffer[0].RefWorldTexelSize, 1e-6);
    float MinRadiusWorld = max(SettingsBuffer.PCSSMinFilterRadiusTexels, 0.0) * RefWorldTexelSizeRef;

    // Interpret LightSize as an angular diameter (degrees).
    const float LightAngularDiameter = max(LightBuffer.LightSize, 0.0);
    const float LightAngularRadiusRad = radians(LightAngularDiameter * 0.5);
    const float LightRadiusScale = tan(LightAngularRadiusRad);

    if (SettingsBuffer.PCSSMinFilterMaxAngularDiameter > 0.0)
    {
        const float MinFilterScale = saturate(LightAngularDiameter / SettingsBuffer.PCSSMinFilterMaxAngularDiameter);
        MinRadiusWorld *= MinFilterScale;
    }

    const float MaxPenumbraWorldRef = max(ShadowSplitsBuffer[0].MaxPCSSRadiusWorld, MinRadiusWorld);
    const float MaxSearchRadiusWorldRef = max(ShadowSplitsBuffer[0].MaxPCSSSearchWorld, MinRadiusWorld);
    float MaxPenumbraWorld = MaxPenumbraWorldRef;
    float MaxSearchRadiusWorld = MaxSearchRadiusWorldRef;
    if (SettingsBuffer.PCSSMaxPenumbraWorld > 0.0)
    {
        MaxPenumbraWorld = min(MaxPenumbraWorld, SettingsBuffer.PCSSMaxPenumbraWorld);
    }
    if (SettingsBuffer.PCSSMaxSearchDistanceWorld > 0.0)
    {
        MaxSearchRadiusWorld = min(MaxSearchRadiusWorld, SettingsBuffer.PCSSMaxSearchDistanceWorld);
    }

    const float SearchAngularDiameter = (SettingsBuffer.PCSSBlockerSearchAngularDiameter > 0.0)
        ? SettingsBuffer.PCSSBlockerSearchAngularDiameter
        : LightAngularDiameter;
    const float SearchAngularRadiusRad = radians(SearchAngularDiameter * 0.5);
    const float SearchRadiusScale = tan(SearchAngularRadiusRad);

    // PCSS Step 1: Find blockers in a search region (UV space)
    float SearchRadiusWorld = SettingsBuffer.PCSSBlockerSearchScale * SearchRadiusScale * max(ReceiverDepthLS - CascadeSplit.NearPlane, 0.0);
    SearchRadiusWorld = clamp(SearchRadiusWorld, MinRadiusWorld, MaxSearchRadiusWorld);
    float2 SearchRadiusUV = SearchRadiusWorld / CascadeExtentsXY;

    const float2 BlockerInfo = ComputeBlockerDepth(CascadeIndex, FilterSetup, SearchRadiusUV, DebugFallbackHit);
    if (BlockerInfo.y < 1.0)
    {
        return 1.0;
    }

    // PCSS Step 2: Penumbra radius based on average blocker depth
    const float AvgBlockerDepthGlobalLS = BlockerInfo.x;

    // Global shadow-space Z can be oriented either towards or away from the light (depending on ShadowMatrix),
    // while cascade depth is always increasing away from the light. Use the cascade Z scale sign to keep the
    // receiver-blocker separation positive.
    const float DepthDirPCSS = (CascadeSplit.Scale.z >= 0.0) ? 1.0 : -1.0;
    const float ReceiverBlockerSeparation = max((ReceiverDepthGlobalLS - AvgBlockerDepthGlobalLS) * DepthDirPCSS, 0.0);

    float PenumbraWorld = SettingsBuffer.PCSSRadiusScale * LightRadiusScale * ReceiverBlockerSeparation;
    const float PenumbraWorldUnclamped = PenumbraWorld;
    PenumbraWorld = clamp(PenumbraWorld, MinRadiusWorld, MaxPenumbraWorld);
    DebugPCSSClamp = (PenumbraWorldUnclamped > 0.0) ? saturate(PenumbraWorld / PenumbraWorldUnclamped) : 1.0;
    float2 FilterRadiusUV = PenumbraWorld / CascadeExtentsXY;

    // PCSS Step 3: Filter the shadow using the estimated penumbra size
    return ShadowAmountPCSS(CascadeIndex, FilterSetup, FilterRadiusUV, DebugFallbackHit);

#elif FILTER_MODE_PCF

    // PCF using a random disk (Poisson or Vogel)
    return ShadowAmountDiscPCF(CascadeIndex, FilterSetup, DebugFallbackHit);

#else
    
    // Fallback to a single sample 
    return ShadowAmountSimple(CascadeIndex, FilterSetup);

#endif
}

float ComputeShadow(float3 PositionWS, float3 Normal, float DepthVS, uint2 Pixel, inout uint CascadeIndex, out float DebugValue)
{
    // Calculate view depth (stable across jitter; used for ViewZ cascade selection).
    const float  ViewPosZ           = max(dot(PositionWS - CameraBuffer.PositionWS, CameraBuffer.Forward), 0.0);
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

        if (all(CascadePosition <= (0.5 + CASCADE_UV_EPSILON)))
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

    // Safety for ViewZ-based selection: the chosen cascade can be correct in Z but still not contain the point in XY
    // (tight frustum / reconstruction error). Walk towards coarser cascades until the point is inside to avoid
    // out-of-bounds sampling (black quads / seams).
#if !SELECT_CASCADE_FROM_PROJECTION
    {
        FCascadeSplit Selected = ShadowSplitsBuffer[CascadeIndex];
        float3 CascadePosition = (ProjectionPosition + Selected.Offsets.xyz) * Selected.Scale.xyz;

        [branch]
        if (!IsInsideCascadeUVW(CascadePosition))
        {
            // NOTE: CascadeIndex is dynamic, so the compiler can't always deduce the loop bounds for unrolling.
            // This loop runs at most (NUM_SHADOW_CASCADES-1) iterations, so a regular loop is fine here.
            [loop]
            for (uint Index = CascadeIndex + 1; Index < NUM_SHADOW_CASCADES; ++Index)
            {
                FCascadeSplit Candidate = ShadowSplitsBuffer[Index];
                float3 CandidatePos = (ProjectionPosition + Candidate.Offsets.xyz) * Candidate.Scale.xyz;

                if (IsInsideCascadeUVW(CandidatePos))
                {
                    CascadeIndex = Index;
                    break;
                }
            }
        }
    }
#endif

#if ENABLE_FIRST_CASCADE_ONLY
    if (CascadeIndex > 0)
    {
        return 1.0;
    }
#endif

    DebugValue = 0.0;

    float DebugPCSSClamp = 1.0;
    uint DebugFallbackHit = 0;

    // Calculate shadow factor
    float ShadowAmount = CascadeShadowAmount(CascadeIndex, PositionWS, Normal, ProjectionPosition, DebugPCSSClamp, DebugFallbackHit);

    FCascadeSplit CascadeSplit = ShadowSplitsBuffer[CascadeIndex];

    float DistToEdge = 1.0;
    float DebugTransition = 0.0;
    float DebugContainment = 1.0;

    {
        float3 CascadePosition = ComputeCascadeUVWFromMatrix(CascadeIndex, PositionWS);
        float3 CascadeAbs = abs(CascadePosition * 2.0 - 1.0);

        DistToEdge = 1.0 - max(max(CascadeAbs.x, CascadeAbs.y), CascadeAbs.z);
        const float EdgeWidth = (CascadeSplit.TransitionMarginTexels / max(float(SettingsBuffer.ShadowMapSize), 1.0)) * 2.0;
        DebugContainment = (DistToEdge >= EdgeWidth) ? 1.0 : 0.0;
    }

// Blend between this and next cascade
#if ENABLE_CASCADE_BLENDING && !ENABLE_FIRST_CASCADE_ONLY
    if (CascadeIndex != (NUM_SHADOW_CASCADES - 1))
    {
        const FCascadeSplit NextSplitData = ShadowSplitsBuffer[CascadeIndex + 1];
        float NextSplit  = CascadeSplit.Split;
        const float TransitionWidth = max(max(CascadeSplit.TransitionWidthViewZ, NextSplitData.TransitionWidthViewZ), 1e-6);
        float FadeFactor = (NextSplit - ViewPosZ) / TransitionWidth;

        const float EdgeMarginTexels = max(CascadeSplit.TransitionMarginTexels, NextSplitData.TransitionMarginTexels);
        const float EdgeWidth = (EdgeMarginTexels / max(float(SettingsBuffer.ShadowMapSize), 1.0)) * 2.0;
        const float EdgeFade = DistToEdge / max(EdgeWidth, 1e-6);
        const float BlendFactor = min(FadeFactor, EdgeFade);

        DebugTransition = saturate(1.0 - BlendFactor);

        [branch]
        if (BlendFactor <= 1.0)
        {
            float NextDebugClamp = 1.0;
            uint NextFallbackHit = 0;
            const float NextSplitVisibility = CascadeShadowAmount(CascadeIndex + 1, PositionWS, Normal, ProjectionPosition, NextDebugClamp, NextFallbackHit);
            const float LerpAmount = smoothstep(0.0, 1.0, BlendFactor);
            ShadowAmount = lerp(NextSplitVisibility, ShadowAmount, LerpAmount);
            DebugFallbackHit = max(DebugFallbackHit, NextFallbackHit);
        }
    }
#endif

    // Optional fade-out beyond max shadow distance.
    if (SettingsBuffer.ShadowMaxDistance > 0.0)
    {
        float Fade = 1.0;
        const float FadeBand = max(SettingsBuffer.ShadowMaxDistanceFade, 0.0);
        if (FadeBand > 0.0)
        {
            Fade = saturate((SettingsBuffer.ShadowMaxDistance - ViewPosZ) / FadeBand);
        }
        else
        {
            Fade = (ViewPosZ <= SettingsBuffer.ShadowMaxDistance) ? 1.0 : 0.0;
        }
        ShadowAmount = lerp(1.0, ShadowAmount, Fade);
    }

    if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_CASCADE_INDEX)
    {
        DebugValue = (NUM_SHADOW_CASCADES > 1) ? (float(CascadeIndex) / float(NUM_SHADOW_CASCADES - 1)) : 0.0;
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_CASCADE_TRANSITION)
    {
        DebugValue = DebugTransition;
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_FILTER_MARGIN)
    {
    #if FILTER_MODE_PCSS
        DebugValue = saturate(CascadeSplit.PCSSMarginTexels / 64.0);
    #else
        DebugValue = saturate(CascadeSplit.PCFMarginTexels / 64.0);
    #endif
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_PCSS_RADIUS_CLAMP)
    {
        DebugValue = saturate(1.0 - DebugPCSSClamp);
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_CASCADE_UPDATED)
    {
        DebugValue = CascadeSplit.CascadeUpdatedThisFrame;
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_CONTAINMENT)
    {
        DebugValue = DebugContainment;
    }
    else if (SettingsBuffer.ShadowDebugMode == SHADOW_DEBUG_CASCADE_FALLBACK)
    {
        DebugValue = (DebugFallbackHit != 0) ? 1.0 : 0.0;
    }

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
        if (SettingsBuffer.ShadowDebugMode != SHADOW_DEBUG_NONE)
        {
            ShadowDebugBuffer[Pixel] = float4(0.0, 0.0, 0.0, 1.0);
        }
        return;
    }

    const float2 PixelCenter   = float2(Pixel) + 0.5;
    const float2 TexCoord      = PixelCenter / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    // Reconstruct using unjittered matrices for temporal stability:
    // - Depth is unaffected by jitter (only x/y shift), so we can unjitter NDC.xy and use the unjittered inverse VP.
    const float X = TexCoord.x * 2.0 - 1.0 - CameraBuffer.Jitter.x;
    const float Y = (1.0 - TexCoord.y) * 2.0 - 1.0 - CameraBuffer.Jitter.y;
    const float4 ProjectedPos  = float4(X, Y, Depth, 1.0);
    const float4 WorldPos4     = mul(ProjectedPos, CameraBuffer.ViewProjectionInvUnjittered);
    const float3 PositionWS    = WorldPos4.xyz / WorldPos4.w;
    const float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0));
    const float3 Normal        = UnpackNormal(GBufferNormal);

    // Cascade-index is written to for debug purposes
    uint CascadeIndex = 0;
    float DebugValue = 0.0;

    // Calculate the Shadow
    const float ShadowAmount = ComputeShadow(PositionWS, Normal, Depth, Pixel, CascadeIndex, DebugValue);
    Output[Pixel] = ShadowAmount;

    if (SettingsBuffer.ShadowDebugMode != SHADOW_DEBUG_NONE)
    {
        ShadowDebugBuffer[Pixel] = float4(DebugValue, DebugValue, DebugValue, 1.0);
    }

    // Output debug-information needed when visualizing the cascades
#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}
