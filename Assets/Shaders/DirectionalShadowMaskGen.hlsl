#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

#define NUM_THREADS (16)

#ifndef ENABLE_DEBUG
    #define ENABLE_DEBUG (0)
#endif

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Soft shadows settings

#define NUM_PCF_SAMPLES (16)
#define PCF_RADIUS      (0.04f)
#define ROTATE_SAMPLES  (1)
#define USE_HAMMERSLY   (1)

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// Cascaded Shadow Mapping Settings

#define SELECT_CASCADE_FROM_PROJECTION (1)
#define USE_RECEIVER_PLANE_DEPTH_BIAS  (0)
#define BLEND_CASCADES                 (1)
#define CASCADE_FADE_FACTOR            (0.1f)

// Camera and Light
ConstantBuffer<FCamera>           CameraBuffer : register(b0);
ConstantBuffer<FDirectionalLight> LightBuffer  : register(b1);

// Shadow information
StructuredBuffer<FCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<FCascadeSplit>    ShadowSplitsBuffer   : register(t1);

// G-Buffer
Texture2D<float>  DepthBuffer  : register(t2);
Texture2D<float3> NormalBuffer : register(t3);

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

/*///////////////////////////////////////////////////////////////////////////////////////////////*/
// ShadowMap Helpers

float2 GetPCFSample(uint Index)
{
#if USE_HAMMERSLY
    return (Hammersley2(Index, NUM_PCF_SAMPLES) * 2.0f) - 1.0f;
#else
#if (NUM_PCF_SAMPLES == 16)
    return PoissonDisk16[Index];
#elif (NUM_PCF_SAMPLES == 32)
    return PoissonDisk32[Index];
#elif (NUM_BLOCKER_SAMPLES == 64)
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

// TODO: Use a texture array for cascades
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

float PCFDirectionalLight(
    uint CascadeIndex,
    float2 TexCoords,
    float BiasedDepth,
    float PenumbraRadius,
    inout uint RandomSeed)
{

    float Shadow = 0;
    for (int Sample = 0; Sample < NUM_PCF_SAMPLES; ++Sample)
    {
        float2 RandomDirection = GetPCFSample(Sample);        
#if ROTATE_SAMPLES
        float    Theta = NextRandom(RandomSeed) * PI_2;
        float2x2 RandomRotationMatrix = float2x2(
            float2(cos(Theta), -sin(Theta)),
            float2(sin(Theta),  cos(Theta)));

        RandomDirection = mul(RandomDirection, RandomRotationMatrix);
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

float CalculateDirectionalLightShadow(
    uint CascadeIndex,
    float3 WorldPosition,
    float3 Normal,
    inout uint RandomSeed)
{
    const float3 ShadowCoords = GetShadowCoords(CascadeIndex, WorldPosition);   
    const float  RadiusScale  = ShadowSplitsBuffer[CascadeIndex].Scale.x;
    
    // Biased depth
    const float NDotL = saturate(dot(Normal, -LightBuffer.Direction)); 
#if USE_RECEIVER_PLANE_DEPTH_BIAS
    const float PlaneRecieverBias = ComputeReceiverPlaneDepthBias();
    
    const float2 TexelSize   = (1.0f / shadowMapSize);
    const float  BiasedDepth = (ShadowCoords.z - Bias);
#else
    const float ShadowBias  = max(LightBuffer.MaxShadowBias * (1.0f - NDotL), LightBuffer.ShadowBias) + 0.0001f;
    const float BiasedDepth = (ShadowCoords.z - ShadowBias);
#endif

    const float Radius = (PCF_RADIUS * RadiusScale);
    return PCFDirectionalLight(CascadeIndex, ShadowCoords.xy, BiasedDepth, Radius, RandomSeed);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(FComputeShaderInput Input)
{
    const uint2 Pixel = Input.DispatchThreadID.xy;
   
    // Discard pixels not rendered to the GBuffer
    float3 GBufferNormal = NormalBuffer.Load(int3(Pixel, 0)).rgb;
    if (length(GBufferNormal) == 0)
    {
        Output[Pixel] = 1.0f;
        return;
    }

    const float  Depth         = DepthBuffer.Load(int3(Pixel, 0)); 
    const float2 TexCoord      = (float2(Pixel) + Float2(0.5f)) / float2(CameraBuffer.ViewportWidth, CameraBuffer.ViewportHeight);
    const float  ViewPosZ      = Depth_ProjToView(Depth, CameraBuffer.ProjectionInverse);
    const float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInverse);
    const float3 Normal        = UnpackNormal(GBufferNormal);

#if SELECT_CASCADE_FROM_PROJECTION
    const float3 ProjectionPosition = mul(float4(WorldPosition, 1.0f), LightBuffer.ShadowMatrix).xyz;
#endif
    
    // Calculate the Current cascade
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
        if(all(CascadePosition < 0.45f))
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
    float ShadowAmount = CalculateDirectionalLightShadow(CascadeIndex, WorldPosition, Normal, RandomSeed);

#if BLEND_CASCADES
    float NextSplit  = ShadowSplitsBuffer[CascadeIndex].Split;
    float SplitSize  = (CascadeIndex == 0) ? NextSplit : (NextSplit - ShadowSplitsBuffer[CascadeIndex - 1].Split);
    float FadeFactor = (NextSplit - ViewPosZ) / SplitSize;
  
#if SELECT_CASCADE_FROM_PROJECTION
    const float4 Offsets = ShadowSplitsBuffer[CascadeIndex].Offsets;
    const float4 Scale   = ShadowSplitsBuffer[CascadeIndex].Scale;

    float3 CascadePosition = ProjectionPosition + Offsets.xyz;
    CascadePosition *= Scale.xyz;
    CascadePosition  = abs(CascadePosition * 2.0f - 1.0f);

    float DistToEdge = 1.0f - max(max(CascadePosition.x, CascadePosition.y), CascadePosition.z);
    FadeFactor = max(DistToEdge, FadeFactor);
#endif

    [branch]
    if(FadeFactor <= CASCADE_FADE_FACTOR && CascadeIndex != (NUM_SHADOW_CASCADES - 1))
    {
        float3 NextSplitVisibility = CalculateDirectionalLightShadow(CascadeIndex + 1, WorldPosition, Normal, RandomSeed);

        float LerpAmount = smoothstep(0.0f, CASCADE_FADE_FACTOR, FadeFactor);
        ShadowAmount = lerp(NextSplitVisibility, ShadowAmount, LerpAmount);
    }
#endif
    
    Output[Pixel] = ShadowAmount;

#if ENABLE_DEBUG
    CascadeIndexTex[Pixel] = CascadeIndex;
#endif
}