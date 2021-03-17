#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "RayTracingHelpers.hlsli"

// Global RootSignature
RaytracingAccelerationStructure Scene : register(t0, space0);

TextureCube<float4> Skybox          : register(t1, space0);
Texture2D<float4>   GBufferAlbedo   : register(t2, space0);
Texture2D<float4>   GBufferNormal   : register(t3, space0);
Texture2D<float4>   GBufferDepth    : register(t4, space0);
Texture2D<float4>   GBufferMaterial : register(t5, space0);

Texture2DArray<float4> BlueNoiseTex : register(t6, space0);

Texture2D<float4> MaterialTextures[128] : register(t7, space0);

ConstantBuffer<Camera>        CameraBuffer : register(b0, space0);
ConstantBuffer<LightInfoData> LightInfo    : register(b1, space0);
ConstantBuffer<RandomData>    RandomBuffer : register(b2, space0);

RWTexture2D<float4> ColorDepth : register(u0, space0);
RWTexture2D<float4> RayPDF     : register(u1, space0);

#define NUM_SAMPLES 1

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();
    uint2 TexCoord           = DispatchIndex.xy;
    uint2 FullResolution     = DispatchDimensions.xy * 2;
    
    uint2 NoiseCoord = DispatchIndex.xy % 64;
    uint  BlueNoise  = (uint)BlueNoiseTex.Load(int4(NoiseCoord, 0, 0)).r * 255.0f;
    BlueNoise = BlueNoise + RandomBuffer.FrameIndex;
    
    uint2 GBufferCoord = TexCoord * 2 + uint2(BlueNoise & 1, (BlueNoise >> 1) & 1);
    
    // Discard rays not rasterized
    float Depth = GBufferDepth.Load(int3(GBufferCoord, 0)).r;
    if (Depth >= 1.0f)
    {
        ColorDepth[TexCoord] = float4(Float3(0.0f), Depth);
        RayPDF[TexCoord]     = Float4(0.0f);
        return;
    }
    
    float2 UV = float2(GBufferCoord) / float2(FullResolution);
    float3 WorldPosition = PositionFromDepth(Depth, UV, CameraBuffer.ViewProjectionInverse);
    float3 WorldNormal   = GBufferNormal.Load(int3(GBufferCoord, 0)).rgb;
    
    float3 Albedo          = GBufferAlbedo.Load(int3(GBufferCoord, 0)).rgb;
    float3 SampledMaterial = GBufferMaterial.Load(int3(GBufferCoord, 0)).rgb;
    float  Roughness       = clamp(SampledMaterial.r, MIN_ROUGHNESS, MAX_ROUGHNESS);

    float3 N = UnpackNormal(WorldNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    uint Seed = RandomInit(DispatchIndex.xy, DispatchDimensions.x, RandomBuffer.FrameIndex % 32);
    
    float2 Xi  = Hammersley(RandomBuffer.FrameIndex % 32, 32);
    float Rnd0 = RandomFloatNext(Seed);
    float Rnd1 = RandomFloatNext(Seed);
    Xi.x = frac(Xi.x + Rnd0);
    Xi.y = frac(Xi.y + Rnd1);
    
    float3 H = ImportanceSampleGGX(Xi, Roughness, N);
    float3 L = normalize(reflect(-V, H));
    
    float3 FinalColor = Float3(0.0f);
    float3 FinalRay   = Float3(0.0f);
    float  FinalPDF   = 0.0f;
    
    float NdotL = saturate(dot(N, L));
    if (NdotL > 0.0f)
    {
        RayDesc Ray;
        Ray.Origin    = WorldPosition + (N * RAY_OFFSET);
        Ray.Direction = L;
        Ray.TMin      = 0.0f;
        Ray.TMax      = 1000.0f;

        RayPayload PayLoad;
        PayLoad.T = Ray.TMax;
        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);
        
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
        
        float D        = DistributionGGX(N, H, Roughness);
        float Spec_PDF = saturate(NdotH / (4.0f * HdotV));
        float3 Li  = PayLoad.Color;
        FinalColor = Li;
        FinalRay   = L * PayLoad.T;
        FinalPDF   = Spec_PDF;
    }

    ColorDepth[TexCoord] = float4(FinalColor, Depth);
    RayPDF[TexCoord]     = float4(FinalRay, FinalPDF);
}