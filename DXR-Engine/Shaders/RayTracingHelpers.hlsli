#ifndef RAY_TRACING_HELPERS
#define RAY_TRACING_HELPERS

float3 WorldHitPosition()
{
    return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

#endif