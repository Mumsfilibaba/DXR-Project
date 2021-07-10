#include "Structs.hlsli"
#include "Random.hlsli"
#include "Helpers.hlsli"
#include "Poisson.hlsli"
#include "Halton.hlsli"

#define NUM_THREADS (16)

// Camera and Light
ConstantBuffer<Camera>           CameraBuffer : register(b0);
ConstantBuffer<DirectionalLight> LightBuffer  : register(b1);

// Shadow information
StructuredBuffer<SCascadeMatrices> ShadowMatricesBuffer : register(t0);
StructuredBuffer<SCascadeSplit>    ShadowSplitsBuffer   : register(t1);

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

// Samplers
SamplerState Sampler : register(s0);

// Cascaded Shadow Mapping
#define NUM_BLOCKER_SAMPLES (64)
#define NUM_PCF_SAMPLES     (64)

#define BLEND_CASCADES  (0)
#define BAND_PERCENTAGE (0.15f)
#define ROTATE_SAMPLES  (0)
#define ENABLE_PCSS     (1)

// Based on: http://developer.download.nvidia.com/whitepapers/2008/PCSS_Integration.pdf
float SearchWidth(float LightSize, float LightNear, float ReciverZ)
{
    return abs(LightSize * (ReciverZ - LightNear) / ReciverZ);
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
        RandomDirection = OneToMinusOne(CranleyPatterssonRotation(MinusOneToOne(RandomDirection), RandomSeed));
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
    
    const float NEAR = 0.5f;
    
#if ENABLE_PCSS
    // Why 0.5f
    float BlockerSearchRadius = SearchWidth(LightSize, NEAR, ShadowCoords.z);
    float BlockerDistance = FindBlockerDistance(ShadowMap, ShadowCoords.xy, CompareDepth, BlockerSearchRadius, Scale, RandomSeed);
    if (BlockerDistance == -1.0f)
    {
        return 1.0f;
    }

    float PenumbraWidth  = (ShadowCoords.z - BlockerDistance) / BlockerDistance;
    float PenumbraRadius = PenumbraWidth * LightSize * NEAR;

    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, PenumbraRadius, Scale, RandomSeed);
#else
    return PCFDirectionalLight(ShadowMap, ShadowCoords.xy, CompareDepth, 0.002f, Scale, RandomSeed);
#endif
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

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    const uint2 Pixel = Input.DispatchThreadID.xy;
    
    uint Width;
    uint Height;
    Output.GetDimensions(Width, Height);
    
    float2 UV = float2(Pixel) / float2(Width, Height);
    
    const float  Depth = DepthTex.Load(int3(Pixel, 0));
    const float3 ViewPosition  = PositionFromDepth(Depth, UV, CameraBuffer.ProjectionInverse);
    const float3 WorldPosition = PositionFromDepth(Depth, UV, CameraBuffer.ViewProjectionInverse);
    float3 GBufferNormal = NormalTex.Load(int3(Pixel, 0)).rgb;
    
    if (length(GBufferNormal) == 0)
    {
        Output[Pixel] = 1.0f;
        return;
    }
    
    const float3 N = UnpackNormal(GBufferNormal);
    
    float3 L = normalize(-LightBuffer.Direction);
    float  ShadowBias = max(LightBuffer.MaxShadowBias * (1.0f - (max(dot(N, L), 0.0f))), LightBuffer.ShadowBias);
    
    float CascadeWeights[NUM_SHADOW_CASCADES];
    for (int i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        CascadeWeights[i] = 0.0f;
    }
    
    float CurrentNearPlane = 0.01f;
    for (i = 0; i < NUM_SHADOW_CASCADES; i++)
    {
        float fCurrentSplit = ShadowSplitsBuffer[i].Split;
        if (ViewPosition.z < fCurrentSplit)
        {
#if BLEND_CASCADES
            float Range = View.z - CurrentNearPlane;
            float SplitRange = fCurrentSplit - CurrentNearPlane;
        
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
        
        CurrentNearPlane = fCurrentSplit;
    }
    
    uint RandomSeed = InitRandom(Pixel, Width, 0);
    
    float fLightSize = LightBuffer.LightSize;
    float fShadow = 0.0f;
    float fBaseScale = 1.0f;
    float fLightNear = 0.0f;
    if (CascadeWeights[0] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(0, WorldPosition);
        float3 LightViewPos = GetLightViewPos(0, WorldPosition);
        float LightFar = ShadowSplitsBuffer[0].FarPlane;
        fShadow += PCSSDirectionalLight(ShadowCascade0, ProjCoords, LightViewPos, fLightSize, fLightNear, LightFar, fBaseScale, ShadowBias, RandomSeed) * CascadeWeights[0];
    }
    if (CascadeWeights[1] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(1, WorldPosition);
        float3 LightViewPos = GetLightViewPos(1, WorldPosition);
        float LightFar = ShadowSplitsBuffer[1].FarPlane;
        fShadow += PCSSDirectionalLight(ShadowCascade1, ProjCoords, LightViewPos, fLightSize, fLightNear, LightFar, fBaseScale, ShadowBias, RandomSeed) * CascadeWeights[1];
    }
    if (CascadeWeights[2] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(2, WorldPosition);
        float3 LightViewPos = GetLightViewPos(2, WorldPosition);
        float LightFar = ShadowSplitsBuffer[2].FarPlane;
        fShadow += PCSSDirectionalLight(ShadowCascade2, ProjCoords, LightViewPos, fLightSize, fLightNear, LightFar, fBaseScale, ShadowBias, RandomSeed) * CascadeWeights[2];
    }
    if (CascadeWeights[3] > 0.0f)
    {
        float3 ProjCoords   = GetShadowCoords(3, WorldPosition);
        float3 LightViewPos = GetLightViewPos(3, WorldPosition);
        float LightFar = ShadowSplitsBuffer[3].FarPlane;
        fShadow += PCSSDirectionalLight(ShadowCascade3, ProjCoords, LightViewPos, fLightSize, fLightNear, LightFar, fBaseScale, ShadowBias, RandomSeed) * CascadeWeights[3];
    }
    
    Output[Pixel] = fShadow;
}