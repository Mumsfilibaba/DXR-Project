#include "PBRCommon.hlsli"

cbuffer TransformBuffer : register(b0, space0)
{
	float4x4 Transform;
};

cbuffer CameraBuffer : register(b1, space0)
{
	float4x4 ViewProjection;
};

float4 VSMain(float3 Position : POSITION0) : SV_Position
{
	return mul(mul(float4(Position, 1.0f), Transform), ViewProjection);
}

float4 PSMain() : SV_Target
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}