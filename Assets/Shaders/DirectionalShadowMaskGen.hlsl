#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

#define NUM_THREADS 16

#ifndef ENABLE_DEBUG
    #define ENABLE_DEBUG 0
#endif

#define FILTER_MODE_PCF_GRID 1
#define FILTER_MODE_PCF_POISSION_DISC 0

// Soft shadows settings
#define USE_GRID_PCF 1
#define USE_HAMMERSLY 0
#define USE_COMPARISON_SAMPLER 1

#define FILTER_SIZE 16
#define MAX_FILTER_SIZE 16
#define NUM_PCF_SAMPLES 16
#define PCF_RADIUS 0.04
#define ROTATE_SAMPLES 0

// Cascaded Shadow Mapping Settings
#define SELECT_CASCADE_FROM_PROJECTION 1
#define USE_RECEIVER_PLANE_DEPTH_BIAS 0
#define BLEND_CASCADES 1
#define CASCADE_FADE_FACTOR 0.1

// Camera and Light
#if SHADER_LANG == SHADER_LANG_MSL
ConstantBuffer<FCamera>           CameraBuffer : register(b2);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b3);
#else
ConstantBuffer<FCamera>           CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);
#endif

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
#if USE_COMPARISON_SAMPLER
SamplerComparisonState ShadowSamplerPoint : register(s0);
SamplerComparisonState ShadowSamplerLinear : register(s1);
#else
SamplerState ShadowSamplerPoint : register(s0);
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ShadowMap Helpers

float2 GetPCFSample(uint Index)
{
#if USE_HAMMERSLY
    return (Hammersley2(Index, NUM_PCF_SAMPLES) * 2.0) - 1.0;
#else
#if (NUM_PCF_SAMPLES == 16)
    return PoissonDisk16[Index];
#elif (NUM_PCF_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_PCF_SAMPLES == 64)
    return PoissonDisk64[Index];
#elif (NUM_PCF_SAMPLES == 128)
    return PoissonDisk128[Index];
#endif
#endif
}

float2 RotatePoisson(float2 Sample, float Random0, float Random1)
{
    return OneToMinusOne(CranleyPatterssonRotation(MinusOneToOne(Sample), Random0, Random1));
}

float3 GetShadowCoords(uint CascadeIndex, float3 World)
{
    float4 LightClipSpacePos = mul(float4(World, 1.0f), ShadowMatricesBuffer[CascadeIndex].ViewProj);
    float3 ProjCoords = LightClipSpacePos.xyz / LightClipSpacePos.w;
    ProjCoords.xy = (ProjCoords.xy * 0.5f) + 0.5f;
    ProjCoords.y  = 1.0f - ProjCoords.y;
    return ProjCoords;
}

float2 ComputeReceiverPlaneDepthBias(float3 TexCoordDX, float3 TexCoordDY)
{
    float2 BiasUV;
    BiasUV.x = (TexCoordDY.y * TexCoordDX.z) - (TexCoordDX.y * TexCoordDY.z);
    BiasUV.y = (TexCoordDX.x * TexCoordDY.z) - (TexCoordDY.x * TexCoordDX.z);
    BiasUV  *= 1.0f / ((TexCoordDX.x * TexCoordDY.y) - (TexCoordDX.y * TexCoordDY.x));
    return BiasUV;
}

float GetShadowMapSize()
{
    uint Width;
    uint Height;
    uint Elements;
    ShadowCascades.GetDimensions(Width, Height, Elements);
    return (float)Width;
}

float PCFDirectionalLight(uint CascadeIndex, float2 TexCoords, float BiasedDepth, float PenumbraRadius, inout uint RandomSeed)
{
#if USE_GRID_PCF
    const float2 MaxFilterSize = float(MAX_FILTER_SIZE) / abs(ShadowSplitsBuffer[0].Scale.xy);
    const float2 FilterSize    = clamp(min(float2(FILTER_SIZE, FILTER_SIZE), MaxFilterSize) * abs(ShadowSplitsBuffer[CascadeIndex].Scale.xy), 1.0, float(MAX_FILTER_SIZE));

    float Result = 0.0;
    
    [branch]
    if (FilterSize.x > 1.0 || FilterSize.y > 1.0)
    {
        const float ShadowMapSize = GetShadowMapSize();
        const float TexelSize     = 1.0 / ShadowMapSize;

        const float2 ShadowTexel   = TexCoords.xy * ShadowMapSize;
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
                const float2 Offset           = float2(SampleX, SampleY) * TexelSize;
                const float2 CurrentTexCoords = TexCoords + Offset;

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

                float Sample = 0.0;
            #if USE_COMPARISON_SAMPLER
                Sample = ShadowCascades.SampleCmpLevelZero(ShadowSamplerPoint, float3(CurrentTexCoords, CascadeIndex), BiasedDepth);
            #else
                const float Depth = ShadowCascades.SampleLevel(ShadowSamplerPoint, float3(CurrentTexCoords, CascadeIndex), 0.0f);
                if (Depth <= BiasedDepth)
                {
                    Sample = 1.0;
                }
            #endif

                Result += Sample * Weight;
            }
        }

        const float NumSamples = FilterSize.x * FilterSize.y;
        Result = Result / NumSamples;
    }
    else
    {
    #if USE_COMPARISON_SAMPLER
        Result = ShadowCascades.SampleCmpLevelZero(ShadowSamplerLinear, float3(TexCoords, CascadeIndex), BiasedDepth);
    #else
        const float Depth = ShadowCascades.SampleLevel(ShadowSamplerLinear, float3(TexCoords, CascadeIndex), 0.0f);
        if (Depth <= BiasedDepth)
        {
            Result = 1.0;
        }
    #endif
    }

#if USE_COMPARISON_SAMPLER
    return saturate(Result);
#else
    return 1.0 - saturate(Result);
#endif
#else
    float Shadow = 0.0;
    const float NumSamples = float(NUM_PCF_SAMPLES);

    for (int Sample = 0; Sample < NUM_PCF_SAMPLES; ++Sample)
    {
        float2 RandomDirection = GetPCFSample(Sample);

    #if ROTATE_SAMPLES
        float Theta    = NextRandom(RandomSeed) * PI_2;
        float CosTheta = cos(Theta);
        float SinTheta = sin(Theta);

        const float2x2 RandomRotationMatrix = float2x2(float2(CosTheta, -SinTheta), float2(SinTheta, CosTheta));
        RandomDirection = mul(RandomDirection, RandomRotationMatrix);
    #endif

        RandomDirection *= PenumbraRadius;
        
    #if USE_COMPARISON_SAMPLER
        const float Depth = SampleCascadeCmp(CascadeIndex, TexCoords + Offset, BiasedDepth);
        Shadow += Depth;
    #else
        const float Depth = SampleCascade(CascadeIndex, TexCoords + Offset);
        if (Depth < BiasedDepth)
        {
            Shadow += 1.0;
        }
    #endif
    }

#if USE_COMPARISON_SAMPLER
    return saturate(Shadow / NumSamples);
#else
    return 1.0 - saturate(Shadow / NumSamples);
#endif
#endif
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

    const float RadiusScale = ShadowSplitsBuffer[CascadeIndex].Scale.x;
    const float Radius      = (PCF_RADIUS * RadiusScale);
    return PCFDirectionalLight(CascadeIndex, ShadowPosition.xy, BiasedDepth, Radius, RandomSeed);
}

float ComputeShadow(float3 WorldPosition, float3 Normal, float Depth, inout uint CascadeIndex, inout uint RandomSeed)
{
    // Calculate z-position in view-space
    const float ViewPosZ = Depth_ProjToView(Depth, CameraBuffer.ProjectionInv);

#if SELECT_CASCADE_FROM_PROJECTION
    const float3 ProjectionPosition = mul(float4(WorldPosition, 1.0), LightBuffer.ShadowMatrix).xyz;
#endif
    
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
    const float2 TexCoord      = (float2(Pixel) + Float2(0.5f)) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
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