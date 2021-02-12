#ifndef RAY_TRACING_HELPERS
#define RAY_TRACING_HELPERS

struct RayPayload
{
    float3 Color;
    uint   CurrentDepth;
};

float3 WorldHitPosition()
{
    return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

#endif