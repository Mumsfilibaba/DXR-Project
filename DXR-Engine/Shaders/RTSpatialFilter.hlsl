#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"
#include "RayTracingHelpers.hlsli"

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

RWTexture2D<float4> HistoryOutput : register(u0);
RWTexture2D<float4> Output        : register(u1);
RWTexture2D<float2> Moments       : register(u2);

static const float2 PoissonDisk[NUM_SAMPLES] =
{
    float2(-0.94201624, -0.39906216),
    float2(0.94558609, -0.76890725),
    float2(-0.094184101, -0.92938870),
    float2(0.34495938, 0.29387760),
    float2(-0.91588581, 0.45771432),
    float2(-0.81544232, -0.87912464),
    float2(-0.38277543, 0.27676845),
    float2(0.97484398, 0.75648379),
    float2(0.44323325, -0.97511554),
    float2(0.53742981, -0.47373420),
    float2(-0.26496911, -0.41893023),
    float2(0.79197514, 0.19090188),
    float2(-0.24188840, 0.99706507),
    float2(-0.81409955, 0.91437590),
    float2(0.19984126, 0.78641367),
    float2(0.14383161, -0.14100790)
};

groupshared float3 SharedResults[NUM_THREADS][NUM_THREADS];

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;

    float4 GBufferAlbedo   = InGBufferAlbedo[TexCoords];
    float4 GBufferNormals  = InGBufferNormals[TexCoords];
    float4 GBufferMaterial = InGBufferMaterial[TexCoords];
    
    float3 Albedo    = GBufferAlbedo.rgb;
    float  Roughness = clamp(GBufferMaterial.r, MIN_ROUGHNESS, MAX_ROUGHNESS);
    float  Metallic  = GBufferMaterial.g;
    
    uint Width;
    uint Height;
    InColorDepth.GetDimensions(Width, Height);
    
    float2 UV = float2(TexCoords) / float2(Width, Height);
    float4 ColorDepth    = InColorDepth[TexCoords];
    float3 WorldPosition = PositionFromDepth(ColorDepth.w, UV, CameraBuffer.ViewProjectionInverse);
    
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    float3 N = UnpackNormal(GBufferNormals.xyz);
    
    float3 F0 = Float3(0.04f);
    F0 = lerp(F0, Albedo, Metallic);
    
    float3 AvgHitPoint = Float3(0.0f);
    float  AvgLength   = 0.0f;
    float3 Result      = Float3(0.0f);
    float3 ResultSqrd  = Float3(0.0f);
    float3 WeightSum   = Float3(0.01f);
    float2 NewMoments  = Float2(0.0f);
    
    uint2 Pixel = uint2(TexCoords.x, TexCoords.y);
    uint Seed = RandomInit(TexCoords.xy, 2, RandomBuffer.FrameIndex);
    
    float3 Min = Float3(FLT32_MAX);
    float3 Max = Float3(-FLT32_MAX);
    
    const float NumSamples  = float(NUM_SAMPLES);
    const float KernelWidth = lerp(1.0f, 8.0f, min(Roughness, 0.35f) / 0.35f);
    for (int y = 0; y < NUM_SAMPLES; y++)
    {
        float2 Xi  = Hammersley(y, NUM_SAMPLES);
        float Rnd0 = RandomFloatNext(Seed);
        float Rnd1 = RandomFloatNext(Seed);
        Xi.x = frac(Xi.x + Rnd0);
        Xi.y = frac(Xi.y + Rnd1);
        Xi = (Xi - 0.5f) * 2.0f;
        
        float2 Rnd = trunc(Xi * KernelWidth);
        int2 LocalTexCoord = TexCoords + int2(Rnd);

        float3 Li = InColorDepth[LocalTexCoord].rgb;
        
        float4 RayPDF = InRayPDF[LocalTexCoord];
        AvgLength += length(RayPDF.xyz);
        
        float3 L = normalize(RayPDF.xyz);
        AvgHitPoint += L;
        
        float3 H = normalize(V + L);
    
        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        
        if (NdotL > 0.0f)
        {
            float  D = DistributionGGX(N, H, Roughness);
            float  G = GeometrySmithGGX_IBL(N, L, V, Roughness);
            float3 F = FresnelSchlick(F0, V, H);
            float3 Numer = F * G;
            float3 Denom = max(Float3(4.0f * NdotL * NdotV), Float3(0.0001f));
    
            float3 Spec_BRDF = Numer / Denom;
            float  Spec_PDF  = RayPDF.a;
    
            float3 Weight      = NdotL * Spec_BRDF / Spec_PDF;
            float3 LocalResult = Li * Weight;
            Result     += LocalResult;
            ResultSqrd += LocalResult * LocalResult;
            WeightSum  += Weight;
            
            Min = min(LocalResult, Min);
            Max = max(LocalResult, Max);
            
            float LocalLuma = Luma(LocalResult);
            NewMoments.x += LocalLuma;
            NewMoments.y += LocalLuma * LocalLuma;
        }
    }
    
    float3 RGBMean = Result / NumSamples;
    float3 M1 = Result * Result / ((NumSamples - 1.0f) * NumSamples * NumSamples);
    float3 M2 = ResultSqrd / (NumSamples * (NumSamples - 1.0f));
    
    Result = Result / WeightSum;
    
    AvgLength   = AvgLength   / NumSamples;
    AvgHitPoint = AvgHitPoint / NumSamples;
    
    NewMoments.x = (NewMoments.x * NewMoments.x) / ((NumSamples - 1.0f) * NumSamples * NumSamples);
    NewMoments.y = (NewMoments.y) / (NumSamples * (NumSamples - 1.0f));
    
    float3 RGBDeviation = sqrt(abs(M2 - M1));
    float3 RGBMin = RGBMean - RGBDeviation;
    float3 RGBMax = RGBMean + RGBDeviation;
    
    //float3 Min = Float3(FLT32_MAX);
    //float3 Max = Float3(-FLT32_MAX);
    //for (int y = -1; y <= 1; y++)
    //{
    //    for (int x = -1; x <= 1; x++)
    //    {
    //        float3 Value = HistoryOutput[TexCoords + int2(x, y)].rgb;
    //        Min = min(Value, Min);
    //        Max = max(Value, Max);
    //    }
    //}
    
    GroupMemoryBarrierWithGroupSync();
    
    float HistoryUsage = 0.95f;
    float MomentsUsage = 0.6f;
    
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
    float4 HistorySample1 = HistoryOutput[HistoryTexCoords1];
    float4 HistorySample  = (HistorySample0 + HistorySample1) * 0.5f;
    float3 ClampedValue   = clamp(HistorySample.rgb, Min, Max);

    float2 HistoryMoments = Moments[TexCoords];
    float2 ResultMoments  = HistoryMoments * MomentsUsage + NewMoments * (1.0f - MomentsUsage);
    Moments[TexCoords] = ResultMoments;
    
    float3 Color   = ClampedValue.rgb * HistoryUsage + Result * (1.0f - HistoryUsage);
    float Variance = ResultMoments.y - ResultMoments.x * ResultMoments.x;
    
    HistoryOutput[TexCoords] = float4(Color, AvgLength);
    
    float4 RayPDF = InRayPDF[TexCoords];
    float3 L = normalize(RayPDF.xyz);
    float3 H = normalize(V + L);
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    
    float  D = DistributionGGX(N, H, Roughness);
    float  G = GeometrySmithGGX_IBL(N, L, V, Roughness);
    float3 F = FresnelSchlick(F0, V, H);
    float3 Numer = F * G;
    float3 Denom = max(Float3(4.0f * NdotL * NdotV), Float3(0.0001f));
    
    float3 Diff_BRDF = Albedo * INV_PI;
    float  Diff_PDF  = NdotL * INV_PI;

    float3 Ks = F;
    float3 Kd = (Float3(1.0f) - Ks) * (1.0f - Metallic);
    
    float3 Spec_BRDF = Numer / Denom;
    float  Spec_PDF  = RayPDF.a;
    
    //Color = Color * NdotL * Spec_BRDF / saturate(Spec_PDF + 0.0001f);
    Color = Color * NdotL * (Spec_BRDF * Ks + Diff_BRDF * Kd) / saturate((Spec_PDF + Diff_PDF) * 0.5f + 0.0001f);
    Output[TexCoords] = float4(Color, Variance);
}