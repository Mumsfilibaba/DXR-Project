#include "PBRHelpers.hlsli"
#include "Structs.hlsli"
#include "Constants.hlsli"
#include "Random.hlsli"
#include "RayTracingHelpers.hlsli"
#include "RayTracingBindless.hlsli"
#include "RayTracingSER.hlsli"

ConstantBuffer<FCamera>                     CameraBuffer    : register(b0);
ConstantBuffer<FRayTracingSceneConstants>   SceneConstants  : register(b1);

RaytracingAccelerationStructure            Scene           : register(t0);
Texture2D<float4>                          GBufferNormal   : register(t2);
Texture2D<float4>                          GBufferDepth    : register(t3);
Texture2D<float4>                          GBufferMaterial : register(t4);
SamplerState                               GBufferSampler  : register(s0);
TEXTURE_FORMAT_UNKNOWN RWTexture2D<float4> OutTexture      : register(u0);

[shader("raygeneration")]
void RayGen()
{
    uint3 DispatchIndex      = DispatchRaysIndex();
    uint3 DispatchDimensions = DispatchRaysDimensions();

    float2 TexCoord = (float2(DispatchIndex.xy) + 0.5) / float2(DispatchDimensions.xy);
    float  Depth    = GBufferDepth.SampleLevel(GBufferSampler, TexCoord, 0).r;

    if (Depth >= 1.0)
    {
        OutTexture[DispatchIndex.xy] = float4(0.0, 0.0, 0.0, -1.0);
        return;
    }

    float3 WorldPosition = PositionFromDepth(Depth, TexCoord, CameraBuffer.ViewProjectionInv);
    float3 WorldNormal   = UnpackNormal(GBufferNormal.SampleLevel(GBufferSampler, TexCoord, 0).rgb);

    const float  Roughness = saturate(GBufferMaterial.SampleLevel(GBufferSampler, TexCoord, 0).r);
    const float3 ViewDir   = normalize(WorldPosition - CameraBuffer.PositionWS);

    float3 ReflectDirection;
    if (Roughness < RAY_TRACING_MIRROR_ROUGHNESS_THRESHOLD)
    {
        ReflectDirection = normalize(reflect(ViewDir, WorldNormal));
    }
    else
    {
        uint   Seed = InitRandom(DispatchIndex.xy, DispatchDimensions.x, SceneConstants.FrameIndex);
        float2 Xi   = NextRandom2(Seed);
        float3 H    = ImportanceSampleGGX(Xi, Roughness, WorldNormal);
        
        ReflectDirection = normalize(reflect(ViewDir, H));
    }

    RayDesc Ray;
    Ray.Origin    = WorldPosition + (WorldNormal * RAY_OFFSET);
    Ray.Direction = ReflectDirection;
    Ray.TMin      = 0.0;
    Ray.TMax      = 10000.0;

    SER_RAY_PAYLOAD_STORAGE FRayPayload PayLoad;
    PayLoad.Color = float3(0.0f, 0.0f, 0.0f);
    PayLoad.HitT  = -1.0f;

#if SHADER_BACKEND == SHADER_BACKEND_VULKAN && RAY_TRACING_SHADER_EXECUTION_REORDERING
    HitObjectEXT Hit;
    HitObjectRecordEmptyEXT(Hit);
    HitObjectTraceRayEXT(Hit, Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray.Origin, Ray.TMin, Ray.Direction, Ray.TMax, PayLoad);
    ReorderThreadWithHitObjectEXT(Hit);
    HitObjectExecuteShaderEXT(Hit, PayLoad);
#elif RAY_TRACING_SHADER_EXECUTION_REORDERING
    dx::HitObject Hit = dx::HitObject::TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);
    dx::MaybeReorderThread(Hit);
    dx::HitObject::Invoke(Hit, PayLoad);
#else
    TraceRay(Scene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, 0xff, 0, 0, 0, Ray, PayLoad);
#endif

    OutTexture[DispatchIndex.xy] = float4(PayLoad.Color, PayLoad.HitT);
}
