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

RWTexture2D<float4> HistoryOutput : register(u0);
RWTexture2D<float4> Output        : register(u1);

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

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords = Input.DispatchThreadID.xy;
    
    float3 Result    = Float3(0.0f);
    float3 WeightSum = Float3(0.01f);
    
    float OriginalPDF = InRayPDF[TexCoords].a;
    
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
    
    uint Seed = RandomInit(TexCoords, Width, 0);
    for (int y = 0; y < NUM_SAMPLES; y++)
    {
        float2 Rnd    = trunc(PoissonDisk[y] * 4.0f);
        int2 TexCoord = TexCoords + int2(Rnd);

        float3 Li     = InColorDepth[TexCoord].rgb;
        float4 RayPDF = InRayPDF[TexCoord];
        float3 L = normalize(RayPDF.xyz);
        float3 H = normalize(V + L);
    
        float NdotL = saturate(dot(N, L));
        float NdotV = saturate(dot(N, V));
        
        if (NdotL > 0.0f)
        {
            float  D = DistributionGGX(N, H, Roughness);
            float  G = GeometrySmithGGX_IBL(N, L, V, Roughness);
            float3 F = FresnelSchlick(F0, V, H);
            float3 Numer = F * G;
            float3 Denom = saturate(Float3(4.0f * NdotL * NdotV) + 0.0001f);
    
            float3 Spec_BRDF = Numer / Denom;
            float  Spec_PDF  = RayPDF.a;
    
            float3 Weight      = NdotL * Spec_BRDF / Spec_PDF;
            float3 LocalResult = Li * Weight;
            Result    += LocalResult;
            WeightSum += Weight;
        }
    }
    
    Result = Result / WeightSum;
    
    float3 L = normalize(InRayPDF[TexCoords].xyz);
    float3 H = normalize(V + L);
    float  NdotL = saturate(dot(N, L));
    float  NdotV = saturate(dot(N, V));
    
    float  D = DistributionGGX(N, H, Roughness);
    float  G = GeometrySmithGGX_IBL(N, L, V, Roughness);
    float3 F = FresnelSchlick(F0, V, H);
    float3 Numer = F * G;
    float3 Denom = Float3(4.0f * NdotL * NdotV);
    
    float3 Diff_BRDF = Albedo * INV_PI;
    float  Diff_PDF  = NdotL * INV_PI;

    float3 Ks = F;
    float3 Kd = (Float3(1.0f) - Ks) * (1.0f - Metallic);
    
    float3 Spec_BRDF = Numer / Denom;
    float  Spec_PDF  = OriginalPDF;
    
    Result = Result * NdotL * (Spec_BRDF) / (Spec_PDF);
    //Result = Result * NdotL * (Spec_BRDF * Ks + Diff_BRDF * Kd) / ((Spec_PDF + Diff_PDF) * 0.5f);
    float LocalLuma = Luma(Result);
    
    float3 Min = Float3(0.0f);
    float3 Max = Float3(1.0f);
    for (int y = -1; y <= 1; y++)
    {
        for (int x = -1; x <= 1; x++)
        {
            float3 Value = HistoryOutput[TexCoords + int2(x, y)].rgb;
            Min = min(Value, Min);
            Max = max(Value, Max);
        }
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    float  HistoryUsage  = 0.98f;
    float4 HistorySample = HistoryOutput[TexCoords];
    float3 ClampedValue  = clamp(Result, Min, Max);

    float3 Color = HistorySample.rgb * HistoryUsage + ClampedValue * (1.0f - HistoryUsage);
    float  Scale = HistorySample.a * 0.5f + LocalLuma * 0.5f;
    
    HistoryOutput[TexCoords] = float4(Color, Scale);
    Output[TexCoords] = float4(Color, Scale);
}