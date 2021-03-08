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

ConstantBuffer<Camera>        CameraBuffer : register(b0, space0);
ConstantBuffer<LightInfoData> LightInfo    : register(b1, space0);
ConstantBuffer<RandomData>    RandomBuffer : register(b2, space0);

SamplerState GBufferSampler : register(s0, space0);

RWTexture2D<float4> ColorDepth : register(u0, space0);
RWTexture2D<float4> RayPDF     : register(u1, space0);

#define NUM_SAMPLES 1

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();
    
    float2 TexCoord = float2(DispatchIndex.xy) / float2(DispatchDimensions.xy);
    float  Depth    = GBufferDepth.SampleLevel(GBufferSampler, TexCoord, 0).r;
    float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInverse);
    float3 WorldNormal   = GBufferNormal.SampleLevel(GBufferSampler, TexCoord, 0).rgb;
    
    float3 Albedo          = GBufferAlbedo.SampleLevel(GBufferSampler, TexCoord, 0).rgb;
    float3 SampledMaterial = GBufferMaterial.SampleLevel(GBufferSampler, TexCoord, 0).rgb;
    float  Roughness       = clamp(SampledMaterial.r, MIN_ROUGHNESS, MAX_ROUGHNESS);

    float3 N = UnpackNormal(WorldNormal);
    float3 V = normalize(CameraBuffer.Position - WorldPosition);
    
    float2 Xi = Hammersley(RandomBuffer.FrameIndex, 512);
    
    uint Seed  = RandomInit(DispatchIndex.xy, DispatchDimensions.x, RandomBuffer.FrameIndex);
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
        Ray.TMax      = 10000.0f;

        RayPayload PayLoad;
        PayLoad.T = Ray.TMax;
        TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);
        
        float NdotH = saturate(dot(N, H));
        float HdotV = saturate(dot(H, V));
        
        float D        = DistributionGGX(N, H, Roughness);
        float Spec_PDF = saturate(NdotH / (4.0f * HdotV));
        float3 Li = PayLoad.Color;
        FinalColor = Li;
        FinalRay   = L * PayLoad.T;
        FinalPDF   = Spec_PDF;
    }

    ColorDepth[DispatchIndex.xy] = float4(FinalColor, Depth);
    RayPDF[DispatchIndex.xy]     = float4(FinalRay.xyz, FinalPDF);
}