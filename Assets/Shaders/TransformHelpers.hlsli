#ifndef TRANSFORM_HELPERS_HLSLI
#define TRANSFORM_HELPERS_HLSLI

#include "Structs.hlsli"

float3 TransformPositionWS(FPerObject T, float3 Position)
{
    const float4 V = float4(Position, 1.0);
    return float3(dot(V, T.LocalToWorld[0]), dot(V, T.LocalToWorld[1]), dot(V, T.LocalToWorld[2]));
}

float3 TransformDirectionWS(FPerObject T, float3 Direction)
{
    const float4 V = float4(Direction, 0.0);
    return float3(dot(V, T.LocalToWorld[0]), dot(V, T.LocalToWorld[1]), dot(V, T.LocalToWorld[2]));
}

float3 TransformDirectionInvT(FPerObject T, float3 Direction)
{
    const float4 V = float4(Direction, 0.0);
    return float3(dot(V, T.TransformInvT[0]), dot(V, T.TransformInvT[1]), dot(V, T.TransformInvT[2]));
}

#endif
