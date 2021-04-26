#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"
#include "Random.hlsli"
#include "Halton.hlsli"

#ifndef ENABLE_HALF_RES
    #define ENABLE_HALF_RES 0
#endif

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0);

Texture2D<float4> GBufferNormalTex   : register(t2);
Texture2D<float4> GBufferDepthTex    : register(t3);
Texture2D<float4> GBufferMaterialTex : register(t4);

#if ENABLE_HALF_RES
Texture2DArray<float4> BlueNoiseTex : register(t5);

StructuredBuffer<RayTracingMaterial> Materials : register(t6);

Texture2D<float> DirectionalShadow : register(t7);

Texture2D<float4> MaterialTextures[1024] : register(t8);
#else
StructuredBuffer<RayTracingMaterial> Materials : register(t5);

Texture2D<float> DirectionalShadow : register(t6);

Texture2D<float4> MaterialTextures[1024] : register(t7);
#endif

ConstantBuffer<Camera>     CameraBuffer : register(b0);
ConstantBuffer<RandomData> RandomBuffer : register(b2);

RWTexture2D<float4> ColorDepth : register(u0);
RWTexture2D<float4> RayPDF     : register(u1);

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();
    uint2 TexCoord           = DispatchIndex.xy;
    
#if ENABLE_HALF_RES
    uint2 FullResolution = DispatchDimensions.xy * 2;
#else
    uint2 FullResolution = DispatchDimensions.xy;
#endif
    
#if ENABLE_HALF_RES
    uint  FrameIndex = RandomBuffer.FrameIndex;
    uint2 NoiseCoord = TexCoord & 63;
    float BlueNoise  = BlueNoiseTex.Load(int4(NoiseCoord, FrameIndex, 0)).r;
    uint  PixelIndex = (uint)(BlueNoise * 255.0f) % 4;
#else
    uint PixelIndex = 0;
#endif

#if ENABLE_HALF_RES
    uint2 BaseTexCoord = TexCoord * 2;
#else
    uint2 BaseTexCoord = TexCoord;
#endif
    
    uint2 GBufferCoord = BaseTexCoord + uint2(PixelIndex & 1, (PixelIndex >> 1) & 1);
    
    float  GBufferDepth    = GBufferDepthTex.Load(int3(GBufferCoord, 0)).r;
    float3 GBufferNormal   = GBufferNormalTex.Load(int3(GBufferCoord, 0)).rgb;
    float3 GBufferMaterial = GBufferMaterialTex.Load(int3(GBufferCoord, 0)).rgb;
    
    float2 UV = float2(GBufferCoord) / float2(FullResolution);
    
    float3 WorldPosition = PositionFromDepth(GBufferDepth, UV, CameraBuffer.ViewProjectionInverse);
    float  Roughness     = GBufferMaterial.r;

    float3 N = UnpackNormal(GBufferNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    if (length(GBufferNormal) == 0.0f)
    {
        ColorDepth[TexCoord] = Float4(0.0f);
        RayPDF[TexCoord]     = Float4(0.0f);
        return;
    }
    
    uint Seed = InitRandom(DispatchIndex.xy, 2560, RandomBuffer.FrameIndex);
    
    float2 Xi = Halton23(RandomBuffer.FrameIndex);
    float Rnd0 = NextRandom(Seed);
    float Rnd1 = NextRandom(Seed);
    Xi.x = frac(Xi.x + Rnd0);
    Xi.y = frac(Xi.y + Rnd1);
    
    float3 H = Float3(0.0f);
    if (Roughness > 0.075f)
    {
        H = ImportanceSampleGGX(Xi, Roughness, N);
    }
    else
    {
        H = N;
    }
    
    float3 L = normalize(reflect(-V, H));
    
    float NdotL = saturate(dot(N, L));
    if (NdotL <= 0.0f)
    {
        Xi = Halton23(RandomBuffer.FrameIndex);
        Rnd0 = NextRandom(Seed);
        Rnd1 = NextRandom(Seed);
        Xi.x = frac(Xi.x + Rnd0);
        Xi.y = frac(Xi.y + Rnd1);
    
        H = ImportanceSampleGGX(Xi, Roughness, N);
        L = normalize(reflect(-V, H));
    }
    
    float3 FinalColor = Float3(0.0f);
    float3 FinalRay   = Float3(0.0f);
    float  FinalPDF   = 0.0f;
    
    RayDesc Ray;
    Ray.Origin    = WorldPosition + (N * RAY_OFFSET);
    Ray.Direction = L;
    Ray.TMin      = 0.0f;
    Ray.TMax      = 1000.0f;

    bool Nan = IsNan(Ray.Origin) || IsNan(Ray.Direction);
    if (!Nan)
    {
        RayPayload PayLoad;
        PayLoad.T = Ray.TMax;
        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);
        
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
        
        float D = DistributionGGX(N, H, Roughness);
        float Spec_PDF = D * NdotH / (4.0f * HdotV);
        
        float3 Li  = PayLoad.Color;
        FinalColor = Li;
        FinalRay   = L * PayLoad.T;
        FinalPDF   = 1.0f / Spec_PDF;
        FinalPDF   = isnan(FinalPDF) || isinf(FinalPDF) ? 0.0f : FinalPDF;
    }
    
    ColorDepth[TexCoord] = float4(FinalColor, GBufferDepth);
    RayPDF[TexCoord]     = float4(FinalRay, FinalPDF);
}