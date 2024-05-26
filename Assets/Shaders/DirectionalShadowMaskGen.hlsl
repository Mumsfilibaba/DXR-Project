#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

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

// PCF grid settings
#ifndef FILTER_SIZE
    #define FILTER_SIZE 256
#endif

#ifndef MAX_FILTER_SIZE
    #define MAX_FILTER_SIZE 512
#endif

// Poisson Disc Settings
#ifndef NUM_PCF_SAMPLES
    #define NUM_PCF_SAMPLES 128
#endif

#ifndef ROTATE_SAMPLES
    #define ROTATE_SAMPLES 1
#endif

// Cascaded Shadow Mapping Settings
#ifndef SELECT_CASCADE_FROM_PROJECTION
    #define SELECT_CASCADE_FROM_PROJECTION 1
#endif

#ifndef USE_RECEIVER_PLANE_DEPTH_BIAS
    #define USE_RECEIVER_PLANE_DEPTH_BIAS 0
#endif

#ifndef BLEND_CASCADES
    #define BLEND_CASCADES 1
#endif

#ifndef CASCADE_FADE_FACTOR
    #define CASCADE_FADE_FACTOR 0.1
#endif


// Camera and Light
#if SHADER_LANG == SHADER_LANG_MSL
ConstantBuffer<FCamera> CameraBuffer : register(b2);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b3);
#else
ConstantBuffer<FCamera> CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer : register(b1);
#endif

struct FDirectionalShadowSettings
{
    float FilterSize;
    float MaxFilterSize;
    uint  Padding0;
    uint  Padding1;
};

ConstantBuffer<FDirectionalShadowSettings> SettingsBuffer : register(b2);

// Shadow information
StructuredBuffer<FCascadeSplit> ShadowSplitsBuffer : register(t1);
StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);

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
SamplerComparisonState ShadowSamplerPoint : register(s0);
SamplerComparisonState ShadowSamplerLinear : register(s1);

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ShadowMap Helpers

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

float GetShadowMapSize()
{
    uint Width;
    uint Height;
    uint Elements;
    ShadowCascades.GetDimensions(Width, Height, Elements);
    return (float)Width;
}

float ShadowAmountPoissonDisc(uint CascadeIndex, float2 ShadowPosition, float BiasedDepth, inout uint RandomSeed)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
    #if ROTATE_SAMPLES
        float Theta    = NextRandom(RandomSeed) * PI_2;
        float CosTheta = cos(Theta);
        float SinTheta = sin(Theta);

        const float2x2 RandomRotationMatrix = float2x2(float2(CosTheta, -SinTheta), float2(SinTheta, CosTheta));
    #endif

        const float  ShadowMapSize = GetShadowMapSize();
        const float2 Radius = (FilterSize * 0.5) / ShadowMapSize;

        for (int Sample = 0; Sample < NUM_PCF_SAMPLES; ++Sample)
        {
            float2 RandomDirection = GetPoissonSample(Sample) * Radius;
        #if ROTATE_SAMPLES
            RandomDirection = mul(RandomDirection, RandomRotationMatrix);
        #endif
            Result += ShadowCascades.SampleCmpLevelZero(ShadowSamplerPoint, float3(ShadowPosition + RandomDirection, CascadeIndex), BiasedDepth);
        }

        Result = Result / float(NUM_PCF_SAMPLES);
        return saturate(Result);
    }
    else
    {
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinear, float3(ShadowPosition, CascadeIndex), BiasedDepth);
    }

    return saturate(Result);
}

float ShadowAmountGrid(uint CascadeIndex, float2 ShadowPosition, float BiasedDepth)
{
    const float2 MaxFilterSize = SettingsBuffer.MaxFilterSize / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(SettingsBuffer.FilterSize.xx, MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, SettingsBuffer.MaxFilterSize);

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float ShadowMapSize = GetShadowMapSize();
        const float TexelSize     = 1.0 / ShadowMapSize;

        const float2 ShadowTexel   = ShadowPosition * ShadowMapSize;
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
                const float2 Offset = float2(SampleX, SampleY) * TexelSize;
                const float2 CurrentTexCoords = ShadowPosition + Offset;

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
                const float Sample = ShadowCascades.SampleCmpLevelZero(ShadowSamplerPoint, float3(CurrentTexCoords, CascadeIndex), BiasedDepth);
                Result += Sample * Weight;
            }
        }

        const float NumSamples = FilterSize.x * FilterSize.y;
        Result = Result / NumSamples;
    }
    else
    {
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinear, float3(ShadowPosition, CascadeIndex), BiasedDepth);
    }

    return saturate(Result);
}

float CascadeShadowAmount(uint CascadeIndex, float3 ShadowPosition, float3 Normal, inout uint RandomSeed)
{
    ShadowPosition += ShadowSplitsBuffer[CascadeIndex].Offsets.xyz;
    ShadowPosition *= ShadowSplitsBuffer[CascadeIndex].Scale.xyz;

#if USE_RECEIVER_PLANE_DEPTH_BIAS
    const float  PlaneRecieverBias = ComputeReceiverPlaneDepthBias();
    const float2 TexelSize         = 1.0 / ShadowMapSize;
    const float  BiasedDepth       = ShadowPosition.z - Bias;
#else
    // Biased Depth
    const float NDotL       = saturate(dot(Normal, -LightBuffer.Direction)); 
    const float ShadowBias  = max(LightBuffer.MaxShadowBias * (1.0 - NDotL), LightBuffer.ShadowBias) + 0.0001;
    const float BiasedDepth = ShadowPosition.z - ShadowBias;
#endif

#if FILTER_MODE_PCF_GRID
    return ShadowAmountGrid(CascadeIndex, ShadowPosition.xy, BiasedDepth);
#elif FILTER_MODE_PCF_POISSION_DISC
    return ShadowAmountPoissonDisc(CascadeIndex, ShadowPosition.xy, BiasedDepth, RandomSeed);
#else
    return 1.0;
#endif
}

float ComputeShadow(float3 WorldPosition, float3 Normal, float Depth, inout uint CascadeIndex, inout uint RandomSeed)
{
    // Calculate z-position in view-space
    const float  ViewPosZ = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);
    const float3 ProjectionPosition = mul(float4(WorldPosition, 1.0), LightBuffer.ShadowMatrix).xyz;
    
    // Find current cascade
    CascadeIndex = NUM_SHADOW_CASCADES - 1;
    for (int Index = CascadeIndex; Index >= 0; --Index)
    {
    #if SELECT_CASCADE_FROM_PROJECTION
        const float4 Offsets = ShadowSplitsBuffer[Index].Offsets;
        const float4 Scale   = ShadowSplitsBuffer[Index].Scale;

        float3 CascadePosition = ProjectionPosition + Offsets.xyz;
        CascadePosition *= Scale.xyz;
        CascadePosition  = abs(CascadePosition - 0.5);
        
        if(all(CascadePosition <= 0.5))
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
    
    // Calculate shadow factor
    float3 ShadowPosition = ProjectionPosition;
    float  ShadowAmount = CascadeShadowAmount(CascadeIndex, ShadowPosition, Normal, RandomSeed);

    // Blend between this and next cascade
#if BLEND_CASCADES
    float NextSplit  = ShadowSplitsBuffer[CascadeIndex].Split;
    float SplitSize  = (CascadeIndex == 0) ? NextSplit : (NextSplit - ShadowSplitsBuffer[CascadeIndex - 1].Split);
    float FadeFactor = (NextSplit - ViewPosZ) / SplitSize;
  
#if SELECT_CASCADE_FROM_PROJECTION
    const float4 Offsets = ShadowSplitsBuffer[CascadeIndex].Offsets;
    const float4 Scale   = ShadowSplitsBuffer[CascadeIndex].Scale;

    float3 CascadePosition = ProjectionPosition + Offsets.xyz;
    CascadePosition *= Scale.xyz;
    CascadePosition  = abs(CascadePosition * 2.0 - 1.0);

    float DistToEdge = 1.0 - max(max(CascadePosition.x, CascadePosition.y), CascadePosition.z);
    FadeFactor = max(DistToEdge, FadeFactor);
#endif

    [branch]
    if(FadeFactor <= CASCADE_FADE_FACTOR && CascadeIndex != (NUM_SHADOW_CASCADES - 1))
    {
        const float NextSplitVisibility = CascadeShadowAmount(CascadeIndex + 1, ShadowPosition, Normal, RandomSeed);
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
        Output[Pixel] = 1.0f;
        return;
    }

    const float  Depth         = DepthBuffer.Load(int3(Pixel, 0)); 
    const float2 TexCoord      = (float2(Pixel) + 0.5) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    const float3 Normal        = UnpackNormal(GBufferNormal);

    // Random Seed when doing soft shadows 
    uint RandomSeed = InitRandom(Pixel, CameraBuffer.ViewportWidth, 0);
    uint CascadeIndex = 0;

    // Calculate the Shadow
    const float ShadowAmount = ComputeShadow(WorldPosition, Normal, Depth, CascadeIndex, RandomSeed);
    
    // Output to the ShadowMask
    Output[Pixel] = ShadowAmount;

    // Output debug-information needed when visualizing the cascades
#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}