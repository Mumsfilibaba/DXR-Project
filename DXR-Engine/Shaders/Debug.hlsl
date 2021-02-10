#include "Structs.hlsli"

cbuffer TransformBuffer : register(b0, space0)
{
    float4x4 TransformMat;
};

ConstantBuffer<Camera> CameraBuffer : register(b1, space0);

float4 VSMain(float3 Position : POSITION0) : SV_Position
{
    return mul(mul(float4(Position, 1.0f), TransformMat), CameraBuffer.ViewProjection);
}

float4 PSMain() : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}