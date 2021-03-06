#ifndef RAY_TRACING_HELPERS
#define RAY_TRACING_HELPERS

struct RayPayload
{
    float3 Color;
    float  T;
};

struct RandomData
{
    int FrameIndex;
    int Seed;
    int Padding1;
    int Padding2;
};

float3 WorldHitPosition()
{
    return WorldRayOrigin() + (RayTCurrent() * WorldRayDirection());
}

#endif