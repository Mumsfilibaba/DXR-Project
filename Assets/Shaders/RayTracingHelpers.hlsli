#ifndef RAY_TRACING_HELPERS_HLSLI
#define RAY_TRACING_HELPERS_HLSLI

#include "CoreDefines.hlsli"

#define RAY_TRACING_MIRROR_ROUGHNESS_THRESHOLD (0.05f)

#define RAY_TRACING_INSTANCE_MASK_ALL      (0xff)
#define RAY_TRACING_RAY_TYPE_REFLECTION    (0)
#define RAY_TRACING_NUM_RAY_TYPES          (1)
#define RAY_TRACING_MISS_SHADER_REFLECTION (0)

#if RAY_TRACING_OPACITY_MICROMAPS && SHADER_BACKEND == SHADER_BACKEND_D3D12
    #define RAY_QUERY_FLAGS (RAYQUERY_FLAG_ALLOW_OPACITY_MICROMAPS)
#else
    #define RAY_QUERY_FLAGS (RAYQUERY_FLAG_NONE)
#endif

#if RAY_TRACING_OPACITY_MICROMAPS && SHADER_BACKEND == SHADER_BACKEND_VULKAN
    #define RAY_QUERY_ALLOW_OPACITY_MICROMAPS() [[vk::ext_capability(6032)]] [[vk::ext_extension("SPV_KHR_opacity_micromap")]] vk::ext_execution_mode_id(6031, true)
#else
    #define RAY_QUERY_ALLOW_OPACITY_MICROMAPS()
#endif

#if RAY_TRACING_SHADER_EXECUTION_REORDERING
    #define RAY_PAYLOAD [raypayload]
    #define RAY_PAYLOAD_QUALIFY(Member) Member : read(caller) : write(caller, closesthit, miss)
#else
    #define RAY_PAYLOAD
    #define RAY_PAYLOAD_QUALIFY(Member) Member
#endif

struct RAY_PAYLOAD FRayPayload
{
    RAY_PAYLOAD_QUALIFY(float3 Color);
    RAY_PAYLOAD_QUALIFY(float  HitT);
};

float3 WorldHitPosition()
{
    return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

#endif