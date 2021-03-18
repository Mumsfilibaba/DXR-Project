#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Kernels.hlsli"
#include "Random.hlsli"

#define NUM_THREADS 16
#define NUM_SAMPLES 16

ConstantBuffer<Camera>     CameraBuffer : register(b0);
ConstantBuffer<RandomData> RandomBuffer : register(b1);

Texture2D<float4> InColorDepth      : register(t0);
Texture2D<float4> InRayPDF          : register(t1);
Texture2D<float4> InGBufferAlbedo   : register(t2);
Texture2D<float4> InGBufferNormals  : register(t3);
Texture2D<float4> InGBufferMaterial : register(t4);
Texture2D<float2> InGBuffervelocity : register(t5);

Texture2DArray<float4> BlueNoiseTex : register(t6);

RWTexture2D<float4> HistoryOutput : register(u0);
RWTexture2D<float4> Output        : register(u1);
RWTexture2D<float2> Moments       : register(u2);

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

float ValidateSample(float3 N, float3 NeighbourN, float Depth, float NeightbourDepth)
{
    float Diff = abs(Depth - NeightbourDepth);
    return (Diff < 0.0005f && Depth < 1.0f && NeightbourDepth < 1.0f) ? 1.0f : 0.0f;
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords     = Input.DispatchThreadID.xy;
    uint2 HalfTexCoords = TexCoords / 2;

    float4 GBufferAlbedo   = InGBufferAlbedo[TexCoords];
    float4 GBufferNormals  = InGBufferNormals[TexCoords];
    float4 GBufferMaterial = InGBufferMaterial[TexCoords];
    
    float3 Albedo    = GBufferAlbedo.rgb;
    float  Roughness = clamp(GBufferMaterial.r, MIN_ROUGHNESS, MAX_ROUGHNESS);
    float  Metallic  = GBufferMaterial.g;
    
    uint HalfWidth;
    uint HalfHeight;
    InColorDepth.GetDimensions(HalfWidth, HalfHeight);
    
    uint Width  = HalfWidth * 2;
    uint Height = HalfHeight * 2;
    
    // UV in full resolution since backbuffer is
    float2 UV = float2(TexCoords) / float2(Width, Height);
    float4 ColorDepth = InColorDepth[HalfTexCoords];
    
    float3 WorldPosition = PositionFromDepth(ColorDepth.w, UV, CameraBuffer.ViewProjectionInverse);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    float3 N = UnpackNormal(GBufferNormals.xyz);
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    float3 AvgHitPoint = Float3(0.0f);
    float  AvgLength   = 0.0f;
    float3 WeightSum   = Float3(0.01f);
    float3 Moment1     = Float3(0.0f);
    float3 Moment2     = Float3(0.0f);
    float2 NewMoments  = Float2(0.0f);
    
    float3 Min = Float3(FLT32_MAX);
    float3 Max = Float3(0.0f);
    
    uint Seed  = InitRandom(TexCoords.xy, Width, RandomBuffer.FrameIndex);
    uint PxIdx = TexCoords.x % 2 + (TexCoords.y % 2) * 2;
    
    float ActualSamples = 0.0f;
    float KernelWidth   = lerp(1.0f, 8.0f, min(Roughness, 0.35f) / 0.35f);
    for (int y = 0; y < NUM_SAMPLES; y++)
    {
        int Rnd0 = (TexCoords.x + NextRandomInt(Seed)) % 64;
        int Rnd1 = (TexCoords.y + NextRandomInt(Seed)) % 64;
        
        const int2 Pixel = int2(Rnd0, Rnd1);
        const int TextureIndex = y & 63;
        
        float4 BlueNoise = BlueNoiseTex.Load(int4(Pixel, TextureIndex, 0));
        float2 Rnd = trunc(mad(BlueNoise.xy, 2.0f, -1.0f) * KernelWidth);
        //float2 Rnd = trunc(PoissonSamples[y] * KernelWidth);
        int2 LocalTexCoord = HalfTexCoords + int2(Rnd);
        
        //float2 Random = float2(NextRandom(Seed), NextRandom(Seed));
        //float2x2 Mat  = float2x2(Random.x, Random.y, -Random.y, Random.x);
        //float2 Offset = RaySamples[PxIdx][y];
        //Offset = mul(Mat, Offset);
        
        //int2 LocalTexCoord = HalfTexCoords + int2(Offset);
        
        float4 RayPDF    = InRayPDF[LocalTexCoord];
        float  RayLength = length(RayPDF.xyz);
        AvgLength += RayLength;
        
        float3 L = RayPDF.xyz / RayLength;
        AvgHitPoint += L;
        
        if (RayPDF.a > 0.0f)
        {
            float3 H = normalize(V + L);
            float NdotL = saturate(dot(N, L));
            float NdotV = saturate(dot(N, V));
            float NdotH = saturate(dot(N, H));
            float HdotV = saturate(dot(H, V));
            
            float4 SampleColorDepth = InColorDepth[LocalTexCoord];
            float3 Li = SampleColorDepth.rgb;
            
            float3 SampleN = UnpackNormal(InGBufferNormals[LocalTexCoord * 2].xyz);
            
            float  D = DistributionGGX(N, H, Roughness);
            float  G = GeometrySmithGGX_IBL(N, L, V, Roughness);
            float3 F = FresnelSchlick(F0, V, H);
            float3 Numer = F * G;
            float3 Denom = max(Float3(4.0f * NdotL * NdotV), Float3(0.0001f));
    
            float3 Spec_BRDF = Numer / Denom;
            float  Spec_PDF  = RayPDF.a;
            
            float  Valid  = ValidateSample(N, SampleN, ColorDepth.w, SampleColorDepth.w);
            float3 Weight = Valid * NdotL * Spec_BRDF / saturate(Spec_PDF + 0.0001f);
            float3 LocalResult = saturate(Li * Weight);
            Moment1   += LocalResult;
            Moment2   += LocalResult * LocalResult;
            WeightSum += Weight;
            
            Min = min(Min, LocalResult);
            Max = max(Max, LocalResult);
            
            float LocalLuma = Luma(LocalResult);
            NewMoments.x += LocalLuma;
            NewMoments.y += LocalLuma * LocalLuma;
            
            ActualSamples += 1.0f;
        }
    }
    
    const float Gamma = 1.0f;
    ActualSamples = max(ActualSamples, 1.0f);
    float3 RGBMean      = Moment1 / ActualSamples;
    float3 RGBVariance  = (Moment2 / ActualSamples) - RGBMean * RGBMean;
    float3 RGBDeviation = max(sqrt(abs(RGBVariance)), 0.0001f);
    float3 RGBMin = RGBMean - Gamma * RGBDeviation;
    float3 RGBMax = RGBMean + Gamma * RGBDeviation;
    
    float3 Result = Moment1 / WeightSum;
    
    const float NumSamples = float(NUM_SAMPLES);
    AvgLength   = AvgLength   / NumSamples;
    AvgHitPoint = AvgHitPoint / NumSamples;
    
    float HistoryUsage = 0.9f;
    float MomentsUsage = 0.5f;
    
    float  PreviousAvgLength = HistoryOutput[TexCoords].w;
    float4 CurrentHit  = float4(AvgHitPoint * AvgLength, 1.0f);
    float4 PreviousHit = float4(AvgHitPoint * PreviousAvgLength, 1.0f);
    CurrentHit  = mul(CurrentHit, CameraBuffer.Projection);
    PreviousHit = mul(PreviousHit, CameraBuffer.Projection);
    
    float2 CurrentClipRay  = CurrentHit.xy / CurrentHit.w;
    CurrentClipRay = (CurrentClipRay * float2(0.5f, -0.5f)) + 0.5f;
    
    float2 PreviousClipRay = PreviousHit.xy / PreviousHit.w;
    PreviousClipRay = (PreviousClipRay * float2(0.5f, -0.5f)) + 0.5f;
    
    float2 RayVelocity = CurrentClipRay - PreviousClipRay;
    RayVelocity        = RayVelocity * float2(Width, Height);
    
    float2 GBufferVelocity = InGBuffervelocity[TexCoords];
    GBufferVelocity        = GBufferVelocity * float2(Width, Height);
    
    uint2 HistoryTexCoords0 = max(int2(TexCoords) - int2(GBufferVelocity), int2(0, 0));
    uint2 HistoryTexCoords1 = max(int2(TexCoords) - int2(RayVelocity), int2(0, 0));
    
    float4 HistorySample0 = HistoryOutput[HistoryTexCoords0];
    HistorySample0.rgb = clamp(HistorySample0.rgb, RGBMin, RGBMax);
    
    float3 ClampedSum = Float3(0.0f);
    //float3 RGBDist = (HistorySample0.rgb - RGBMean) / RGBDeviation;
    //float  Weight  = exp2(-10.0f * Luma(RGBDist));
    //ClampedSum += HistorySample0.rgb * Weight;
    
    float4 HistorySample1 = HistoryOutput[HistoryTexCoords1];
    HistorySample1.rgb = clamp(HistorySample1.rgb, RGBMin, RGBMax);
    
    //RGBDist = (HistorySample1.rgb - RGBMean);
    //Weight  = exp2(-10.0f * Luma(RGBDist));
    //ClampedSum += HistorySample1.rgb * Weight;
    ClampedSum = HistorySample0.rgb * 0.4f + HistorySample1.rgb * 0.6f;
    
    float2 HistoryMoments = Moments[TexCoords];
    float2 ResultMoments  = HistoryMoments * MomentsUsage + NewMoments * (1.0f - MomentsUsage);
    Moments[TexCoords] = ResultMoments;
    
    ResultMoments.x = ResultMoments.x / ActualSamples;
    float Variance  = (ResultMoments.y / ActualSamples) - ResultMoments.x * ResultMoments.x;
    
    GroupMemoryBarrierWithGroupSync();
    
    float3 Color = ClampedSum * HistoryUsage + Result * (1.0f - HistoryUsage);
    HistoryOutput[TexCoords] = float4(Color, AvgLength);
    Output[TexCoords]        = float4(Color, Roughness);
}