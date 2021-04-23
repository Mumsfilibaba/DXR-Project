#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Kernels.hlsli"
#include "Random.hlsli"

#define NUM_THREADS (16)
#define NUM_SAMPLES (16)

#define MIN_TEMPORAL_FRAMES (1)
#define MAX_TEMPORAL_FRAMES (64)

#define MAX_ROUGHNESS (0.25f)

#ifndef ENABLE_HALF_RES
    #define ENABLE_HALF_RES 1
#endif

ConstantBuffer<Camera> CameraBuffer : register(b0);

Texture2D<float4> ColorDepthTex       : register(t0);
Texture2D<float4> RayPDFTex           : register(t1);
Texture2D<float4> GBufferAlbedoTex    : register(t2);
Texture2D<float4> GBufferNormalsTex   : register(t3);
Texture2D<float4> GBufferMaterialTex  : register(t4);
Texture2D<float4> GBufferVelocityTex  : register(t5);
Texture2D<float>  GBufferDepthTex     : register(t6);
Texture2D<float4> GBufferPrevNormTex  : register(t7);
Texture2D<float4> GBufferPrevDepthTex : register(t8);

RWTexture2D<float4> Reconstructed  : register(u0);
RWTexture2D<float4> PrevHistory    : register(u1);
RWTexture2D<float4> History        : register(u2);
RWTexture2D<float4> Reflections    : register(u3);

// Blue noise classified 
static const float2 RaySamples[4][16] =
{
    {
        float2(0.f, 0.f), float2(-1.f, -1.f), float2(1.f, 0.f), float2(-2.f, -1.f),
        float2(-2.f, 1.f), float2(0.f, -3.f), float2(2.f, -2.f), float2(1.f, 2.f),
        float2(0.f, 3.f), float2(-4.f, -1.f), float2(0.f, -4.f), float2(-2.f, 3.f),
        float2(-4.f, 1.f), float2(3.f, 1.f), float2(3.f, -3.f), float2(4.f, 0.f)
    },
    {
        float2(0.f, 0.f), float2(1.f, -1.f), float2(0.f, -2.f), float2(-1.f, -2.f),
        float2(-1.f, 2.f), float2(0.f, 2.f), float2(2.f, 0.f), float2(-3.f, 1.f),
        float2(-3.f, -2.f), float2(1.f, -3.f), float2(-3.f, 2.f), float2(3.f, 0.f),
        float2(-2.f, -4.f), float2(1.f, 3.f), float2(-4.f, -2.f), float2(2.f, 3.f)
    },
    {
        float2(0.f, 0.f), float2(-1.f, 0.f), float2(0.f, 1.f), float2(-2.f, 0.f),
        float2(2.f, -1.f), float2(-1.f, -3.f), float2(-3.f, -1.f), float2(2.f, 2.f),
        float2(2.f, -3.f), float2(3.f, -1.f), float2(-3.f, -3.f), float2(-4.f, 0.f),
        float2(-1.f, -4.f), float2(-4.f, 2.f), float2(-3.f, 3.f), float2(4.f, -1.f)
    },
    {
        float2(0.f, 0.f), float2(0.f, -1.f), float2(-1.f, 1.f), float2(-2.f, -2.f),
        float2(1.f, 1.f), float2(1.f, -2.f), float2(-3.f, 0.f), float2(2.f, 1.f),
        float2(-2.f, -3.f), float2(-2.f, 2.f), float2(-1.f, 3.f), float2(1.f, -4.f),
        float2(3.f, -2.f), float2(3.f, 2.f), float2(-4.f, -3.f), float2(0.f, 4.f)
    }
};

float IsValidSample(uint2 TexCoords, float3 N, float FwidthN, float3 NeighbourN, float Depth, float NeightbourDepth)
{
    uint Width;
    uint Height;
    PrevHistory.GetDimensions(Width, Height);
    
    bool Result = (distance(N, NeighbourN) / (FwidthN + 1e-2f)) < 16.0f;
    Result = Result && (TexCoords.x >= 0 && TexCoords.x <= Width && TexCoords.y >= 0 && TexCoords.y <= Height);
    return (Result && Depth < 1.0f && NeightbourDepth < 1.0f) ? 1.0f : 0.0f;
}

float IsValidReprojection(uint2 TexCoords, float3 N, float FwidthN, float3 PreviousN)
{
    uint Width;
    uint Height;
    PrevHistory.GetDimensions(Width, Height);
    
    bool Result = TexCoords.x >= 0 && TexCoords.x <= Width && TexCoords.y >= 0 && TexCoords.y <= Height;
    return (Result && ((distance(N, PreviousN) / (FwidthN + 1e-2f)) < 16.0f)) ? 1.0f : 0.0f;
}

float3 ClipAABB(float3 Min, float3 Max, float3 History)
{
    float3 ClipP  = 0.5f * (Max + Min);
    float3 ClipE  = 0.5f * (Max - Min);
    float3 ClipV  = History - ClipP;
    float3 UnitV  = ClipV.xyz / ClipE;
    float3 UnitA  = abs(UnitV);
    float  UnitMA = max(UnitA.x, max(UnitA.y, UnitA.z));

    if (UnitMA > 1.0f)
    {
        return ClipP + ClipV / UnitMA;
    }
    else
    {
        return History;
    }
}

float4 LoadHistory(int2 TexCoords, float3 N, out float Valid)
{
    uint Width;
    uint Height;
    PrevHistory.GetDimensions(Width, Height);
    float2 Size = float2(Width, Height);
    
    // Sample motion vectors
    float3 GBufferVelocity = GBufferVelocityTex[TexCoords].xyz;
    float  FWidthN = GBufferVelocity.z;
    
    int2 Velocity      = int2(GBufferVelocity.xy * Size);
    int2 HistoryCoords = TexCoords - Velocity;
    
    float4 HistorySample0 = PrevHistory[HistoryCoords];
    float3 PreviousNormal = UnpackNormal(GBufferPrevNormTex[HistoryCoords].xyz);
    
    Valid = IsValidReprojection(HistoryCoords, N, FWidthN, PreviousNormal);
    
    // TODO: Reflection reprojection
    
    return float4(HistorySample0.rgb, lerp(0.0f, HistorySample0.a, Valid));
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;
#if ENABLE_HALF_RES
    uint2 HalfTexCoords = TexCoords / 2;
#else
    uint2 HalfTexCoords = TexCoords;
#endif

    float4 GBufferAlbedo   = GBufferAlbedoTex[TexCoords];
    float4 GBufferNormals  = GBufferNormalsTex[TexCoords];
    float4 GBufferMaterial = GBufferMaterialTex[TexCoords];
    
    float3 Albedo    = GBufferAlbedo.rgb;
    float  Roughness = GBufferMaterial.r;
    float  Metallic  = GBufferMaterial.g;
    
    uint HalfWidth;
    uint HalfHeight;
    ColorDepthTex.GetDimensions(HalfWidth, HalfHeight);
    
#if ENABLE_HALF_RES
    uint Width  = HalfWidth * 2;
    uint Height = HalfHeight * 2;
#else
    uint Width  = HalfWidth;
    uint Height = HalfHeight;
#endif
    
    // UV in full resolution since backbuffer is
    float2 UV = float2(TexCoords) / float2(Width, Height);
    float  Depth = GBufferDepthTex[TexCoords];
    
    float3 WorldPosition = PositionFromDepth(Depth, UV, CameraBuffer.ViewProjectionInverse);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    float3 N = UnpackNormal(GBufferNormals.xyz);
    if (length(GBufferNormals.xyz) == 0.0f)
    {
        Reconstructed[TexCoords] = Float4(0.0f);
        History[TexCoords]       = Float4(0.0f);
        Reflections[TexCoords]   = Float4(0.0f);
        return;
    }
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);

    uint PxIdx = TexCoords.x % 2 + (TexCoords.y % 2) * 2;
    
    int KernelWidth = int(floor(lerp(1.0f, (float)NUM_SAMPLES, min(Roughness, MAX_ROUGHNESS) / MAX_ROUGHNESS)));
    
    float2 LumaMoments = Float2(0.0f);
    float3 WeightSum   = Float3(0.0f);
    float3 Result  = Float3(0.0f);
    float3 Moment1 = Float3(0.0f);
    float3 Moment2 = Float3(0.0f);
    for (int i = 0; i < KernelWidth; i++)
    {
        float2 Offset = RaySamples[PxIdx][i];
        int2   LocalTexCoord = HalfTexCoords + int2(Offset);
            
        float4 RayPDF = RayPDFTex[LocalTexCoord];
        float  RayLength = length(RayPDF.xyz);
        float3 L = RayPDF.xyz / RayLength;

        float3 H = normalize(V + L);

        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
            
        float4 SampleColorDepth = ColorDepthTex[LocalTexCoord];
        float3 Li = saturate(SampleColorDepth.rgb);
            
#if ENABLE_HALF_RES
        uint2 FullLocalTexCoord = LocalTexCoord * 2;
#else
        uint2 FullLocalTexCoord = LocalTexCoord;
#endif
        float3 SampleN = UnpackNormal(GBufferNormalsTex[FullLocalTexCoord].xyz);
        float  FWidthN = GBufferVelocityTex[FullLocalTexCoord].z;
            
        float  D = DistributionGGX(N, H, Roughness);
        float  G = GeometrySmithGGX(N, L, V, Roughness);
        float3 F = FresnelSchlick(F0, V, H);
        float3 Numer = D * F * G;
        float  Denom = max(4.0f * NdotL * NdotV, 1e-6);
            
        float3 Spec_BRDF = Numer / Denom;
        float  Spec_PDF  = RayPDF.a;
            
        float3 Ks = F;
        float3 Kd = (Float3(1.0f) - Ks) * (1.0f - Metallic);
            
        float  Valid  = IsValidSample(FullLocalTexCoord, N, FWidthN, SampleN, Depth, SampleColorDepth.w);
        float3 Weight = Valid * NdotL * Spec_BRDF * Spec_PDF;
        Weight = IsNan(Weight) || IsInf(Weight) ? Float3(0.0f) : Weight;
            
        float3 LocalResult = Li * Weight;
        Result    += LocalResult;
        WeightSum += Weight;
            
        Moment1 += Li;
        Moment2 += Li * Li;

        LumaMoments.x += Luminance(LocalResult);
        LumaMoments.y += LumaMoments.x * LumaMoments.x;
    }
    
    // Statistics of used pixels
    const float VarianceClipSigma = 0.5f;
    float3 Mean   = Moment1 / float(KernelWidth);
    float3 Dev    = sqrt(Moment2 / float(KernelWidth) * Mean * Mean);
    float3 BoxMin = Mean - VarianceClipSigma * Dev;
    float3 BoxMax = Mean + VarianceClipSigma * Dev;
    
    LumaMoments = LumaMoments / float(NUM_SAMPLES);
    
    float4 ColorDepth = ColorDepthTex[HalfTexCoords];
    if (!IsEqual(WeightSum, Float3(0.0f)))
    {
        Result = Result / WeightSum;
    }
    else
    {
        Result = saturate(ColorDepth.rgb);
    }
    
    Reconstructed[TexCoords] = float4(Result, KernelWidth);
    
    float Valid = 0.0f;
    float4 HistorySample = LoadHistory(TexCoords, N, Valid);
    
    float3 Clipped = ClipAABB(BoxMin, BoxMax, HistorySample.rgb);
    
    float NumTemporalFrames = floor(lerp((float)MIN_TEMPORAL_FRAMES,(float) MAX_TEMPORAL_FRAMES, clamp(Roughness, 0.0f, 0.45f) / 0.45f));
    float HistoryLength     = HistorySample.a;
    HistoryLength = min(NumTemporalFrames, HistoryLength + 1.0f);
    
    const float Alpha = lerp(1.0f, max(0.03f, 1.0f / HistoryLength), Valid);
    float3 Color = HistorySample.rgb * (1.0f - Alpha) + Result * Alpha;
    
    History[TexCoords] = float4(Color, HistoryLength);

    float Variance = LumaMoments.y - LumaMoments.x * LumaMoments.x;
    Reflections[TexCoords] = float4(Color, Variance);
}