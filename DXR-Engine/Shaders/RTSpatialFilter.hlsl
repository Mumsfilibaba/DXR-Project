#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Helpers.hlsli"
#include "Constants.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Kernels.hlsli"
#include "Random.hlsli"

#define NUM_THREADS 16
#define NUM_SAMPLES 16

#define MAX_TEMPORAL_FRAMES 64

ConstantBuffer<Camera> CameraBuffer : register(b0);

Texture2D<float4> ColorDepthTex      : register(t0);
Texture2D<float4> RayPDFTex          : register(t1);
Texture2D<float4> GBufferAlbedoTex   : register(t2);
Texture2D<float4> GBufferNormalsTex  : register(t3);
Texture2D<float4> GBufferMaterialTex : register(t4);
Texture2D<float4> GBufferVelocityTex : register(t5);

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

float ValidateSample(float3 N, float FwidthN, float3 NeighbourN, float Depth, float NeightbourDepth)
{
    bool Result = (distance(N, NeighbourN) / (FwidthN + 1e-2f)) < 16.0f;
    return (Result && Depth < 1.0f && NeightbourDepth < 1.0f) ? 1.0f : 0.0f;
}

float ValidateReprojection(uint2 TexCoords, float3 N, float FwidthN, float3 NeighbourN)
{
    uint Width;
    uint Height;
    PrevHistory.GetDimensions(Width, Height);
    
    bool Result = TexCoords.x >= 0 && TexCoords.x <= Width && TexCoords.y >= 0 && TexCoords.y <= Height;
    return (Result && ((distance(N, NeighbourN) / (FwidthN + 1e-2f)) < 16.0f)) ? 1.0f : 0.0f;
}

float4 LoadHistory(int2 TexCoords, float3 N, float3 Min, float3 Max, out float Valid)
{
    uint Width;
    uint Height;
    PrevHistory.GetDimensions(Width, Height);
    float2 Size = float2(Width, Height);
    
    // Sample motion vectors
    float2 GBufferVelocity = GBufferVelocityTex[TexCoords].xy;
    int2 Velocity      = int2(GBufferVelocity * Size);
    int2 HistoryCoords = TexCoords - Velocity;
    
    float4 HistorySample0 = PrevHistory[HistoryCoords];
    HistorySample0.rgb = clamp(HistorySample0.rgb, Min, Max);
    
    float3 HistoryN = UnpackNormal(GBufferNormalsTex[HistoryCoords].xyz);
    float  HistoryFWidthN = GBufferVelocityTex[HistoryCoords].z;
    
    Valid = ValidateReprojection(HistoryCoords, N, HistoryFWidthN, HistoryN);
    
    float History = lerp(0.0f, HistorySample0.a, Valid);
    History = min((float) MAX_TEMPORAL_FRAMES, History + 1.0f);
    return float4(HistorySample0.rgb, History);
}

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void Main(ComputeShaderInput Input)
{
    uint2 TexCoords     = Input.DispatchThreadID.xy;
    uint2 HalfTexCoords = TexCoords / 2;

    float4 GBufferAlbedo   = GBufferAlbedoTex[TexCoords];
    float4 GBufferNormals  = GBufferNormalsTex[TexCoords];
    float4 GBufferMaterial = GBufferMaterialTex[TexCoords];
    
    float3 Albedo    = GBufferAlbedo.rgb;
    float  Roughness = GBufferMaterial.r;
    float  Metallic  = GBufferMaterial.g;
    
    uint HalfWidth;
    uint HalfHeight;
    ColorDepthTex.GetDimensions(HalfWidth, HalfHeight);
    
    uint Width  = HalfWidth * 2;
    uint Height = HalfHeight * 2;
    
    // UV in full resolution since backbuffer is
    float2 UV = float2(TexCoords) / float2(Width, Height);
    float4 ColorDepth = ColorDepthTex[HalfTexCoords];
    
    float3 WorldPosition = PositionFromDepth(ColorDepth.w, UV, CameraBuffer.ViewProjectionInverse);
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
    
    int KernelWidth = int(floor(lerp(1.0f, (float)NUM_SAMPLES, min(Roughness, 0.35f) / 0.35f)));
    
    float3 WeightSum = Float3(0.0f);
    float3 Result    = Float3(0.0f);
    float3 Moment1   = Float3(0.0f);
    float3 Moment2   = Float3(0.0f);
    float3 BoxMin = Float3(FLT32_MAX);
    float3 BoxMax = Float3(0.0f);
    for (int i = 0; i < KernelWidth; i++)
    {
        float2 Offset = RaySamples[PxIdx][i];
        int2   LocalTexCoord = HalfTexCoords + int2(Offset);
            
        float4 RayPDF = RayPDFTex[LocalTexCoord];
        float  RayLength = length(RayPDF.xyz);
        float3 L = RayPDF.xyz / RayLength;

        if (RayPDF.a > 0.0f)
        {
            float3 H = normalize(V + L);

            float NdotL = saturate(dot(N, L));
            float NdotV = saturate(dot(N, V));
            float NdotH = saturate(dot(N, H));
            float HdotV = saturate(dot(H, V));
            
            float4 SampleColorDepth = ColorDepthTex[LocalTexCoord];
            float3 Li = saturate(SampleColorDepth.rgb);
            
            uint2  FullLocalTexCoord = LocalTexCoord * 2;
            float3 SampleN = UnpackNormal(GBufferNormalsTex[FullLocalTexCoord].xyz);
            float  FWidthN = GBufferVelocityTex[FullLocalTexCoord].z;
            
            float  D = DistributionGGX(N, H, Roughness);
            float  G = GeometrySmithGGX(N, L, V, Roughness);
            float3 F = FresnelSchlick(F0, V, H);
            float3 Numer = D * F * G;
            float  Denom = 4.0f * NdotL * NdotV;
    
            float3 Spec_BRDF = Numer / Denom;
            float  Spec_PDF  = RayPDF.a;
            
            float  Valid  = ValidateSample(N, FWidthN, SampleN, ColorDepth.w, SampleColorDepth.w);
            float3 Weight = Valid * NdotL * Spec_BRDF * Spec_PDF;
            Weight = IsNan(Weight) || IsInf(Weight) ? Float3(0.0f) : Weight;
            
            float3 LocalResult = Li * Weight;
            Result    += LocalResult;
            WeightSum += Weight;
            
            Moment1 += Li;
            Moment2 += Li * Li;
            
            BoxMin = min(BoxMin, Li);
            BoxMax = max(BoxMax, Li);
        }
    }
    
    if (!IsEqual(WeightSum, Float3(0.0f)))
    {
        Result = Result / WeightSum;
    }
    else
    {
        Result = saturate(ColorDepth.rgb);
    }
    
    Reconstructed[TexCoords] = float4(Result, 0.0f);
    
    float  Valid = 0.0f;
    float4 HistorySample = LoadHistory(TexCoords, N, BoxMin, BoxMax, Valid);
    float  NumHistorySamples = HistorySample.a;
    float3 Sample            = HistorySample.rgb;
    
    const float HistoryUsage = lerp(1.0f, min(0.98f, 1.0f / NumHistorySamples), Valid);
    float3 Color = Sample * (1.0f - HistoryUsage) + Result * HistoryUsage;
    
    History[TexCoords]     = float4(Color, NumHistorySamples);
    Reflections[TexCoords] = float4(Color, Roughness);
}