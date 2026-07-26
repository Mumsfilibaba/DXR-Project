#ifndef RAY_TRACING_HELPERS_HLSLI
#define RAY_TRACING_HELPERS_HLSLI

#include "CoreDefines.hlsli"

#define RAY_TRACING_MIRROR_ROUGHNESS_THRESHOLD (0.05f)

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